/*
  FluviSafe — teste do corpo de prova com ESP32 e interface web

  LIGAÇÃO (alimente SEMPRE pelo 3V3):
    HX710B VCC -> 3V3      HX710B SCK -> GPIO 18
    HX710B GND -> GND      HX710B OUT -> GPIO 19

  COMO USAR:
    1. Grave este programa (placa "ESP32 Dev Module").
    2. No celular ou notebook, conecte na rede Wi-Fi  FluviSafe-Teste  (senha: fluvisafe).
    3. Abra no navegador:  http://192.168.4.1
    O Monitor Serial (115200) também mostra o endereço.

  A interface (HTML, CSS e JavaScript) está na aba pagina.h.
  A calibração fica salva no ESP32 e continua valendo depois de desligar.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <math.h>
#include "pagina.h"

// ---------- Wi-Fi ----------
// Com WIFI_NOME vazio, o ESP32 cria a própria rede (não precisa de roteador).
// Para usar o Wi-Fi de casa, preencha nome e senha (o ESP32 só conecta em 2,4 GHz).
const char* WIFI_NOME     = "FluviSafe";
const char* WIFI_SENHA    = "Pex@1324";
const char* REDE_PROPRIA  = "FluviSafe";
const char* SENHA_PROPRIA = "12345678";          // mínimo de 8 caracteres

// ---------- sensor ----------
const int  PINO_SCK  = 18;
const int  PINO_DOUT = 19;
const long LIMITE_SALTO = 300000;                 // salto maior que isso em 0,1 s é leitura errada
const int  AMOSTRAS_MEDIA = 20;                   // ~2 s de leituras para zerar e calibrar

WebServer servidor(80);
Preferences memoria;
portMUX_TYPE trava = portMUX_INITIALIZER_UNLOCKED;
String endereco;

// calibração
long  zeroBruto = 0;
float fator = 0.0f;                               // contagens por milímetro de água
bool  zerado = false;
bool  calibrado = false;

// leituras
double filtrado = NAN;
long   ultimoBruto = 0;
bool   temLeitura = false;
const int N_JANELA = 30;
long janela[N_JANELA];
int  janelaPos = 0, janelaN = 0, rejeitadasSeguidas = 0;
unsigned long ultimoPronto = 0;

// diagnóstico: resultado das últimas 50 tentativas
enum Resultado : uint8_t { LEITURA_OK, SEM_RESPOSTA, INVALIDA, SALTO };
const int N_HIST = 50;
uint8_t hist[N_HIST];
int histPos = 0, histN = 0;

// zerar e calibrar acontecem em segundo plano, sem travar a página
enum Acao { NENHUMA, ZERANDO, CALIBRANDO };
Acao acao = NENHUMA;
float alvoMm = 0.0f;
long long somaAcao = 0;
int nAcao = 0;
unsigned long inicioAcao = 0;
String mensagem = "", tipoMensagem = "";

void registrar(Resultado r) {
  hist[histPos] = r;
  histPos = (histPos + 1) % N_HIST;
  if (histN < N_HIST) histN++;
}

void salvar() {
  memoria.putLong("zero", zeroBruto);
  memoria.putFloat("fator", fator);
  memoria.putBool("zerado", zerado);
  memoria.putBool("cal", calibrado);
}

String comVirgula(float v, int casas) {
  String s = String(v, casas);
  s.replace(".", ",");
  return s;
}

// Lê os 24 bits do HX710B. Só é chamada quando OUT já está em nível baixo (dado pronto).
bool lerHX710B(long &valor) {
  uint32_t dado = 0;
  portENTER_CRITICAL(&trava);                     // pulsos curtos: SCK alto por mais de ~50 µs desliga o chip
  for (int i = 0; i < 24; i++) {
    digitalWrite(PINO_SCK, HIGH);
    delayMicroseconds(1);
    dado = (dado << 1) | (digitalRead(PINO_DOUT) ? 1u : 0u);
    digitalWrite(PINO_SCK, LOW);
    delayMicroseconds(1);
  }
  digitalWrite(PINO_SCK, HIGH);                   // 25º pulso: próxima leitura na entrada diferencial, 10 Hz
  delayMicroseconds(1);
  digitalWrite(PINO_SCK, LOW);
  delayMicroseconds(1);
  portEXIT_CRITICAL(&trava);

  if (dado & 0x800000u) dado |= 0xFF000000u;      // negativo (complemento de dois, 24 bits)
  valor = (long)(int32_t)dado;
  // todos os bits em 1 (-1), todos em 0 ou no limite do conversor: sinal de fio ou solda com problema
  return !(valor == -1 || valor == 0 || valor == 0x7FFFFF || valor == -0x800000);
}

long medianaRecente() {
  long v[7];
  int n = janelaN < 7 ? janelaN : 7;
  for (int i = 0; i < n; i++) v[i] = janela[(janelaPos - 1 - i + N_JANELA) % N_JANELA];
  for (int i = 1; i < n; i++) {                   // ordenação simples
    long x = v[i]; int j = i - 1;
    while (j >= 0 && v[j] > x) { v[j + 1] = v[j]; j--; }
    v[j + 1] = x;
  }
  return v[n / 2];
}

void concluirAcao() {
  long media = (long)(somaAcao / nAcao);
  if (acao == ZERANDO) {
    zeroBruto = media;
    zerado = true;
    salvar();
    mensagem = "Sensor zerado.";
    tipoMensagem = "ok";
  } else {
    float k = (float)(media - zeroBruto) / alvoMm;
    if (fabs(k) < 1.0f) {
      mensagem = "A leitura quase não mudou. Confira se o tubo está preso no bico do sensor e no sino, sem vazamento.";
      tipoMensagem = "erro";
    } else {
      fator = k;
      calibrado = true;
      salvar();
      mensagem = "Calibrado com " + comVirgula(alvoMm, 1) + " mm.";
      tipoMensagem = "ok";
    }
  }
  acao = NENHUMA;
  filtrado = NAN;
}

void processar(long v) {
  if (janelaN >= 5 && labs(v - medianaRecente()) > LIMITE_SALTO) {
    if (rejeitadasSeguidas < 3) { rejeitadasSeguidas++; registrar(SALTO); return; }
    janelaN = 0; janelaPos = 0;                   // mudança real e mantida: aceita e recomeça
  }
  rejeitadasSeguidas = 0;
  registrar(LEITURA_OK);
  janela[janelaPos] = v;
  janelaPos = (janelaPos + 1) % N_JANELA;
  if (janelaN < N_JANELA) janelaN++;
  ultimoBruto = v;
  temLeitura = true;
  filtrado = isnan(filtrado) ? (double)v : filtrado + 0.3 * ((double)v - filtrado);

  if (acao != NENHUMA) {
    somaAcao += v;
    nAcao++;
    if (nAcao >= AMOSTRAS_MEDIA) concluirAcao();
  }
}

// ---------- respostas da página ----------
String numOuNull(double v, int casas) {
  if (isnan(v) || isinf(v)) return "null";
  return String(v, casas);
}

void enviarJson(int codigo, const String &json) {
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(codigo, "application/json; charset=utf-8", json);
}

void erro(const String &texto) {
  enviarJson(400, "{\"erro\":\"" + texto + "\"}");
}

String diagnosticoJson() {
  int c[4] = {0, 0, 0, 0};
  for (int i = 0; i < histN; i++) c[hist[i]]++;
  String nivel, texto;
  float taxaOk = histN ? 100.0f * c[LEITURA_OK] / histN : 0;

  double ruido = NAN;
  int n = janelaN < 20 ? janelaN : 20;
  if (n >= 10) {
    double soma = 0, soma2 = 0;
    long ref = janela[(janelaPos - 1 + N_JANELA) % N_JANELA];
    for (int i = 0; i < n; i++) {
      double d = (double)(janela[(janelaPos - 1 - i + N_JANELA) % N_JANELA] - ref);
      soma += d; soma2 += d * d;
    }
    double m = soma / n;
    ruido = sqrt(fmax(0.0, soma2 / n - m * m));
  }

  if (histN < 5) {
    nivel = "aguardando"; texto = "Iniciando a leitura do sensor.";
  } else if (c[SEM_RESPOSTA] > histN * 0.8) {
    nivel = "erro"; texto = "O sensor não responde. Confira o fio OUT no GPIO 19, o VCC no 3V3 e o GND.";
  } else if (c[INVALIDA] + c[SALTO] > histN * 0.2) {
    nivel = "erro"; texto = "O sensor manda leituras impossíveis. Confira as soldas dos pinos do módulo e se SCK e OUT não estão trocados.";
  } else if (n >= 10 && ruido == 0) {
    nivel = "erro"; texto = "A leitura está travada sempre no mesmo número. Confira as soldas do módulo.";
  } else if (taxaOk < 90) {
    nivel = "alerta"; texto = "O sensor funciona, mas " + String(100 - taxaOk, 0) + "% das leituras falham. Firme os jumpers.";
  } else {
    nivel = "ok"; texto = "Sensor respondendo bem.";
  }

  double ruidoMm = (calibrado && !isnan(ruido)) ? fabs(ruido / fator) : NAN;
  return "{\"nivel\":\"" + nivel + "\",\"texto\":\"" + texto + "\",\"validas\":" + String(taxaOk, 0) +
         ",\"ruido\":" + numOuNull(ruido, 0) + ",\"ruido_mm\":" + numOuNull(ruidoMm, 2) + "}";
}

void paginaEstado() {
  double mm = (calibrado && !isnan(filtrado)) ? (filtrado - zeroBruto) / fator : NAN;
  String json = "{";
  json += "\"bruto\":" + (temLeitura ? String(ultimoBruto) : String("null"));
  json += ",\"filtrado\":" + numOuNull(filtrado, 0);
  json += ",\"mm\":" + numOuNull(mm, 2);
  json += ",\"zerado\":" + String(zerado ? "true" : "false");
  json += ",\"calibrado\":" + String(calibrado ? "true" : "false");
  json += ",\"fator\":" + (calibrado ? String(fator, 1) : String("null"));
  json += ",\"ocupado\":" + String(acao != NENHUMA ? "true" : "false");
  json += ",\"mensagem\":\"" + mensagem + "\",\"tipo\":\"" + tipoMensagem + "\"";
  json += ",\"endereco\":\"" + endereco + "\"";
  json += ",\"diag\":" + diagnosticoJson();
  json += "}";
  enviarJson(200, json);
}

void comecarAcao(Acao a) {
  acao = a; somaAcao = 0; nAcao = 0; inicioAcao = millis();
  mensagem = ""; tipoMensagem = "";
}

void paginaZerar() {
  if (acao != NENHUMA) return erro("Aguarde a medição anterior terminar.");
  comecarAcao(ZERANDO);
  enviarJson(200, "{\"ok\":true}");
}

void paginaCalibrar() {
  float mm = servidor.arg("mm").toFloat();
  if (acao != NENHUMA) return erro("Aguarde a medição anterior terminar.");
  if (!zerado) return erro("Zere primeiro, com o corpo de prova fora da água.");
  if (mm < 3 || mm > 60) return erro("Informe um calado entre 3 e 60 mm.");
  alvoMm = mm;
  comecarAcao(CALIBRANDO);
  enviarJson(200, "{\"ok\":true}");
}

void paginaApagar() {
  zerado = false; calibrado = false; zeroBruto = 0; fator = 0;
  salvar();
  mensagem = "Calibração apagada."; tipoMensagem = "ok";
  enviarJson(200, "{\"ok\":true}");
}

void iniciarWiFi() {
  if (strlen(WIFI_NOME) > 0) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_NOME, WIFI_SENHA);
    Serial.print("Conectando ao Wi-Fi ");
    Serial.print(WIFI_NOME);
    unsigned long t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) { delay(300); Serial.print("."); }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      endereco = "http://" + WiFi.localIP().toString();
      if (MDNS.begin("fluvisafe-teste")) MDNS.addService("http", "tcp", 80);
      return;
    }
    Serial.println("Não conectou. Criando a rede própria do ESP32...");
  }
  WiFi.mode(WIFI_AP);
  WiFi.softAP(REDE_PROPRIA, SENHA_PROPRIA);
  endereco = "http://" + WiFi.softAPIP().toString();
}

void mostrarEndereco() {
  Serial.println();
  if (WiFi.getMode() == WIFI_AP) {
    Serial.print("Conecte o celular na rede Wi-Fi \"");
    Serial.print(REDE_PROPRIA);
    Serial.print("\" (senha: ");
    Serial.print(SENHA_PROPRIA);
    Serial.println(")");
  }
  Serial.print("Abra no navegador: ");
  Serial.println(endereco);
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_SCK, OUTPUT);
  digitalWrite(PINO_SCK, LOW);                    // SCK em nível baixo mantém o HX710B ligado
  pinMode(PINO_DOUT, INPUT_PULLUP);               // fio OUT solto vira "sem resposta", não números inventados

  memoria.begin("fluvisafe", false);
  zeroBruto = memoria.getLong("zero", 0);
  fator     = memoria.getFloat("fator", 0.0f);
  zerado    = memoria.getBool("zerado", false);
  calibrado = memoria.getBool("cal", false) && fator != 0.0f;

  iniciarWiFi();
  servidor.on("/", HTTP_GET, []() { servidor.send(200, "text/html; charset=utf-8", PAGINA); });
  servidor.on("/api/estado", HTTP_GET, paginaEstado);
  servidor.on("/api/zerar", HTTP_POST, paginaZerar);
  servidor.on("/api/calibrar", HTTP_POST, paginaCalibrar);
  servidor.on("/api/apagar", HTTP_POST, paginaApagar);
  servidor.onNotFound([]() { servidor.sendHeader("Location", "/"); servidor.send(302, "text/plain", ""); });
  servidor.begin();
  mostrarEndereco();
}

void loop() {
  servidor.handleClient();

  if (digitalRead(PINO_DOUT) == LOW) {            // há leitura pronta
    long v;
    if (lerHX710B(v)) processar(v);
    else registrar(INVALIDA);
    ultimoPronto = millis();
  } else if (millis() - ultimoPronto > 300) {     // a 10 Hz deveria chegar uma leitura a cada ~100 ms
    registrar(SEM_RESPOSTA);
    ultimoPronto = millis();
  }

  if (acao != NENHUMA && millis() - inicioAcao > 6000) {
    acao = NENHUMA;
    mensagem = "Poucas leituras válidas. Resolva o aviso do sensor no topo da página e tente de novo.";
    tipoMensagem = "erro";
  }

  static unsigned long ultimoLembrete = 0;
  if (millis() - ultimoLembrete > 15000) { ultimoLembrete = millis(); mostrarEndereco(); }
  delay(1);
}

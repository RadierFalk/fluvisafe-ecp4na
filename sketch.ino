
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h> 

// --- Configurações do Display OLED ---
#define LARGURA_TELA 128
#define ALTURA_TELA  64
#define ENDERECO_I2C_OLED 0x3C
Adafruit_SSD1306 display(LARGURA_TELA, ALTURA_TELA, &Wire, -1);

// --- Configurações do MPU6050 ---
Adafruit_MPU6050 mpu;

const int pinosCalado[] = {32, 33, 25, 26};
// Ordem: 0=Proa BB, 1=Proa BE, 2=Popa BB, 3=Popa BE

#define NUM_AMOSTRAS 10  // Quantas leituras para calcular a média

// --- Limites de Alerta ---
#define LIMITE_SOBRECARGA  4.2  // metros — acima disso: perigo!
#define LIMITE_BANDA       0.50 // metros — diferença máxima entre bordos
#define LIMITE_FALHA_MIN   0.05 // metros — abaixo disso consideramos sensor com falha

const float DENSIDADE_AGUA = 1.000;


float lerCaladoComFiltro(int pino) {
  long soma = 0;
  for (int i = 0; i < NUM_AMOSTRAS; i++) {
    soma += analogRead(pino);
    delay(5); // pequena pausa entre leituras (5ms × 10 = 50ms total)
  }
  float mediaBruta = soma / (float)NUM_AMOSTRAS;
  // Converte a leitura ADC (0 a 4095) para metros (0 a 5m)
  return (mediaBruta / 4095.0) * 5.0;
}

float calcularAngulo(float ax, float ay, float az) {
  // Ângulo de rolamento (roll) — inclinação lateral do barco
  // Fórmula: atan2(y, sqrt(x² + z²)) convertido para graus
  float angulo = atan2(ay, sqrt(ax * ax + az * az));
  return angulo * (180.0 / M_PI); // converte radianos para graus
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando Sistema Draft Survey v2.0...");

  // Inicializa OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, ENDERECO_I2C_OLED)) {
    Serial.println("ERRO: Falha ao iniciar o OLED!");
    for (;;); // trava aqui se o display falhar
  }

  // Inicializa MPU6050
  if (!mpu.begin()) {
    Serial.println("AVISO: MPU6050 não encontrado. Verifique a fiação I2C.");
    // Não travamos aqui — o sistema pode operar sem o MPU6050
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
    mpu.setGyroRange(MPU6050_RANGE_250_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 iniciado com sucesso.");
  }

  // Tela de boas-vindas
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 15);
  display.println("DRAFT SURVEY v2.0");
  display.setCursor(20, 35);
  display.println("Iniciando...");
  display.display();
  delay(2000);
}

void loop() {
  float calados[4];
  float somaCalados   = 0;
  bool  sensorFalhou  = false;
  int   indiceFalha   = -1;

  // ── 1. LEITURA DOS 4 SENSORES COM FILTRO DE MARULHO ────────
  for (int i = 0; i < 4; i++) {
    calados[i] = lerCaladoComFiltro(pinosCalado[i]);
    somaCalados += calados[i];

    // Envia dados para o Serial Monitor (útil para depuração)
    Serial.print("Sensor ");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(calados[i], 2);
    Serial.print("m  ");
  }
  Serial.println();

  float mediaGeral = somaCalados / 4.0;

  int sensoresAtivos = 0;
  for (int i = 0; i < 4; i++) {
    if (calados[i] > LIMITE_FALHA_MIN) sensoresAtivos++;
  }
  if (sensoresAtivos >= 3) { // pelo menos 3 funcionando
    for (int i = 0; i < 4; i++) {
      if (calados[i] <= LIMITE_FALHA_MIN) {
        sensorFalhou = true;
        indiceFalha  = i;
      }
    }
  }

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float anguloGraus = calcularAngulo(
    a.acceleration.x,
    a.acceleration.y,
    a.acceleration.z
  );

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("FLUVISAFE");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  if (sensorFalhou) {
    display.setCursor(0, 14);
    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);

    const char* nomesSensores[] = {"ProaBB", "ProaBE", "PopaBB", "PopaBE"};
    char msgFalha[22];
    snprintf(msgFalha, sizeof(msgFalha), " FALHA: %s ", nomesSensores[indiceFalha]);
    display.println(msgFalha);
    display.setTextColor(SSD1306_WHITE);

    // Ainda mostra a média dos sensores que funcionam
    display.setCursor(0, 34);
    display.setTextSize(1);
    display.print("Med(3 sens):");
    display.print(mediaGeral, 2);
    display.print("m");
    display.setCursor(0, 50);
    display.print("Angulo:");
    display.print(anguloGraus, 1);
    display.print("'");

  } else if (abs(calados[0] - calados[1]) > LIMITE_BANDA) {
    // Exibe o alerta de banda em destaque
    display.setCursor(0, 14);
    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.println(" ! PERIGO BANDA ! ");
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 30);
    display.setTextSize(1);
    display.print("BB:");
    display.print(calados[0], 1);
    display.print("m BE:");
    display.print(calados[1], 1);
    display.print("m");

    display.setCursor(0, 44);
    display.print("Med:");
    display.print(mediaGeral, 2);
    display.print("m");
    display.setCursor(0, 56);
    display.print("Angulo:");
    display.print(anguloGraus, 1);
    display.print("'");

  } else if (mediaGeral > LIMITE_SOBRECARGA) {
    display.setCursor(0, 14);
    display.setTextSize(1);
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    display.println(" !!! SOBRECARGA !!! ");
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 30);
    display.setTextSize(1);
    display.print("Calado:");
    display.print(mediaGeral, 2);
    display.print("m");

    display.setCursor(0, 44);
    display.print("Limite: 4.20m");

    display.setCursor(0, 56);
    display.print("Angulo:");
    display.print(anguloGraus, 1);
    display.print("'");

  } else {
    // Linha 1: Calado médio em destaque (fonte grande)
    display.setCursor(0, 13);
    display.setTextSize(2);
    display.print(mediaGeral, 2);
    display.print("m");

    // Linha 2: Ângulo de inclinação
    display.setTextSize(1);
    display.setCursor(0, 34);
    display.print("Ang:");
    display.print(anguloGraus, 1);
    display.print("' ");

    // Linha 2 (cont.): Diferença de banda ao vivo
    float difBanda = abs(calados[0] - calados[1]);
    display.print("Dif:");
    display.print(difBanda, 2);
    display.print("m");

    // Linha 3: Mini-leituras individuais dos 4 sensores
    display.setCursor(0, 46);
    display.print("PB:");
    display.print(calados[0], 1);
    display.print(" PE:");
    display.print(calados[1], 1);

    display.setCursor(0, 56);
    display.print("PB:");
    display.print(calados[2], 1);
    display.print(" PE:");
    display.print(calados[3], 1);
  }

  display.display();

  delay(450);
}

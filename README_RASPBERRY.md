# FluviSafe — versão de simulação para Raspberry Pi

Esta etapa troca a comunicação Web Serial do ESP32 por uma API FastAPI executada
no Raspberry Pi. Os quatro calados e o MPU6050 são simulados; cada leitura é
gravada no banco SQLite `data/fluvisafe.db`.

## 1. Copiar os arquivos

Extraia o pacote e copie seu conteúdo para a raiz do clone
`~/Desktop/fluvisafe-ecp4na`.

O arquivo `flivisafe.js` deste pacote deve substituir o arquivo atual. Os
arquivos `flivisafe.html` e `flivisafe.css` existentes permanecem inalterados.

## 2. Preparar o ambiente

```bash
cd ~/Desktop/fluvisafe-ecp4na
source .venv/bin/activate
pip install -r requirements.txt
```

## 3. Iniciar o servidor

```bash
uvicorn app.main:app --host 0.0.0.0 --port 8000
```

No próprio Raspberry, acesse `http://127.0.0.1:8000`.

Em outro computador da mesma rede, descubra o IP:

```bash
hostname -I
```

Depois acesse `http://IP_DO_RASPBERRY:8000`.

## 4. Testar a API

```bash
curl http://127.0.0.1:8000/api/health
curl http://127.0.0.1:8000/api/telemetry
curl "http://127.0.0.1:8000/api/history?limit=5"
```

A documentação interativa fica em `http://IP_DO_RASPBERRY:8000/docs`.

## 5. Cenários disponíveis

O dashboard cria quatro botões: Normal, Banda, Sobrecarga e Falha. Também é
possível mudar o cenário pelo terminal:

```bash
curl -X POST http://127.0.0.1:8000/api/simulation/scenario/normal
curl -X POST http://127.0.0.1:8000/api/simulation/scenario/band
curl -X POST http://127.0.0.1:8000/api/simulation/scenario/overload
curl -X POST http://127.0.0.1:8000/api/simulation/scenario/sensor_failure
```

Encerre o servidor com `Ctrl+C`.

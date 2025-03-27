# 🚀 **Sistema de Monitoramento Inteligente de Lixeiras (SMIL)**

## 📌 **Descrição**
O **SMIL** é um sistema embarcado que monitora o nível de ocupação de lixeiras em tempo real. Ele utiliza sensores e uma interface gráfica em um display OLED para exibir informações detalhadas sobre a ocupação da lixeira, tendências de uso e um modo noturno para reduzir o consumo de energia.

## 🎯 **Funcionalidades**
- 📊 **Monitoramento em Tempo Real**: Exibição do percentual de ocupação da lixeira.
- 📈 **Gráficos e Tendências**: Representação gráfica da ocupação ao longo do tempo.
- 🌙 **Modo Noturno**: Reduz a luminosidade dos LEDs para economia de energia.
- 🎮 **Navegação via Joystick**: Interface intuitiva para alternar entre as telas.

## 🖥️ **Interface do Display**
O sistema exibe quatro seções principais, acessíveis através do joystick:

1. **📌 Tela Principal** – Mostra:
   - Percentual de ocupação da lixeira.
   - Estado do sistema (modo normal ou noturno).
   - Indicadores visuais para alertas.

2. **📊 Tela de Gráficos** – Exibe a variação do nível de ocupação ao longo do tempo.

3. **📉 Tela de Tendências** – Apresenta estatísticas detalhadas, incluindo a média diária de ocupação e previsões futuras.

4. **🌙 Tela de Modo de Operação** – Indica se o sistema está no Modo Normal ou Modo Noturno.

🔄 **Transição entre telas:**
```
Principal → Gráficos → Tendências → Modo de Operação → Principal
```

## 🛠️ **Tecnologias Utilizadas**
- **Microcontrolador:** Raspberry Pi Pico W (BitDogLab)
- **Linguagem:** C
- **Sensores:** Sensor ultrassônico HC-SR04 para medição da ocupação
- **Display:** OLED SSD1306
- **LEDs:** WS2812B para indicadores visuais
- **Interface:** Joystick analógico para navegação

## ⚙️ **Instalação e Uso**
### 1️⃣ **Pré-requisitos**
- [Pico SDK](https://github.com/raspberrypi/pico-sdk) instalado
- Compilador C (GCC para ARM)
- Biblioteca para o display SSD1306
- Biblioteca para os LEDs WS2812B

### 2️⃣ **Clonar o repositório**
```sh
git clone https://github.com/thalyssonDEV/smil.git
cd smil
```

### 3️⃣ **Compilar e carregar o código no Raspberry Pi Pico**
```sh
mkdir build && cd build
cmake ..
make
cp firmware.uf2 /media/pi/RPI-RP2
```

## 📌 **Modo de Operação**
- **Modo Normal:** Brilho ajustável via joystick.
- **Modo Noturno:** LEDs fixos na menor luminosidade e bloqueio de ajuste de brilho.

---
📌 **Desenvolvido por Thalysson** 🚀

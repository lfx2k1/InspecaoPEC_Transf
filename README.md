# Simulador de Inspeção PEC em Transformadores

## Descrição do Projeto
Este projeto implementa um simulador de inspeção não destrutiva em transformadores utilizando Correntes Parasitas Pulsadas (PEC). Através do uso de um joystick, é possível simular o deslocamento de uma sonda sobre a superfície de um transformador. O sistema detecta zonas de falha e fornece feedback visual (LEDs, matriz WS2812, display OLED) e sonoro (buzzer), além de armazenar o histórico de falhas detectadas.

O projeto foi desenvolvido na plataforma educacional **BitDogLab**, integrando diversos periféricos com controle em linguagem C e uso de interrupções com debounce.

## Link de acesso ao vídeo

Link de acesso para o vídeo com a explicação do código e testes na placa BitDogLab: https://youtu.be/Yr2aC2KhNYM

## Funcionalidades
- **Simulação de Inspeção PEC:** Simulação do deslocamento de uma sonda sobre um transformador por meio de joystick analógico.
- **Detecção de Zonas de Falha:** O sistema identifica quando o cursor entra em regiões simuladas com defeitos.
- **Controle de LEDs:** LEDs indicam o status da inspeção (normal, alerta de falha, área inspecionada).
- **Matriz de LEDs WS2812:** Representação visual dinâmica do percurso ou área inspecionada.
- **Display OLED:** Exibe a posição do cursor, conforme o movimento do joystick.
- **Acionamento de Buzzer:** Alarme sonoro é ativado quando o sistema detecta uma falha.
- **Botões A e B:**  
  - **Botão A:** Exibe o histórico de falhas registradas no Serial Monitor.  
  - **Botão B:** Realiza a reinicialização do sistema (reset).

## Hardware Utilizado
- **BitDogLab (RP2040)**
- **Joystick Analógico (eixo X para inspeção)**
- **LED RGB (Na implemetação uiltizou-se apenas o canal vermelho e verde)**
- **Matriz de LEDs WS2812 (endereçável)**
- **Display OLED (via biblioteca SSD1306)**
- **Buzzer**
- **Botões físicos A e B (com interrupção e debounce)**

## Software Utilizado
- **Linguagem de Programação:** C
- **IDE:** Visual Studio Code
- **Extensão:** BitDogLab / Wokwi
- **Compilador:** GCC (para Raspberry Pi Pico - RP2040)
- **Bibliotecas Externas:** `ssd1306.h`, `hardware/pwm.h`, `hardware/adc.h`, entre outras nativas do SDK Pico

## Instruções de Instalação
1. **Clone este repositório:**
   ```bash
   git clone https://github.com/lfx2k1/InspecaoPEC_Transf.git
   cd InspecaoPEC_Transf

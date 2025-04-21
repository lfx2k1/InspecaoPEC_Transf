#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "pico/bootrom.h"

// Definições para utilização no código

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define ENDERECO_DISPLAY 0x3C

#define JOYSTICK_X_PIN 27 // Pino do eixo X do Joystick
#define JOYSTICK_Y_PIN 26 // Pino do eixo Y do Joystick

#define Botao_A 5 // Botão A na BitDogLab
#define Botao_B 6 // Botão B na BitDogLab
#define LED_R 13  // Led Vermelho
#define LED_G 11  // Led Verde
#define LED_B 12  // Led Azul
#define BUZZER 21 // Pino do buzzer

#define IS_RGBW false
#define NUM_LEDS 25
#define WS2812_PIN 7

volatile bool coleta_dados = false; // Variável volátil para coleta de dados
bool cor = true;
int contador_falhas = 0;      // Contador para incrementar valores após a detecção de falhas
bool estava_em_falha = false; // detecta uma ocorrência de fissura

#define MAX_ENTRADAS 4
int historico_falhas[MAX_ENTRADAS] = {0}; // Onde será armazenado as falhas (como um histórico)
int pos_atual = 0;                        // Posição atual para o vetor circular
int total_entradas = 0;                   // Valor que trata-se do total de coletas feitas

// Variáveis globais para a matriz de LEDs
uint32_t buffer_leds[NUM_LEDS] = {0};

// Função para enviar um pixel para a matriz de LEDs
static inline void enviar_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
    sleep_us(80); // Pequeno delay para estabilidade
}

// Função para converter RGB para GRB com redução de brilho
static inline uint32_t converter_rgb_para_grb(uint8_t r, uint8_t g, uint8_t b)
{
    // Reduz o brilho em 95% (multiplica por 0.05)
    r = (uint8_t)(r * 0.05); // para vermelho
    g = (uint8_t)(g * 0.05); // para verde
    b = (uint8_t)(b * 0.05); // para azul
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// Configura a matriz WS2812
void configurar_matriz_leds()
{
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);
}

// Função para limpar a matriz de LEDs (todos desligados)
void limpar_matriz_leds()
{
    for (int i = 0; i < NUM_LEDS; i++)
    {
        buffer_leds[i] = 0; // Define o valor GRB como 0 (desligado)
    }
    for (int i = 0; i < NUM_LEDS; i++)
    {
        enviar_pixel(buffer_leds[i]); // Envia o valor para cada LED
    }
}

// Função para desenhar um quadrado 3x3 na matriz de LEDs
void desenhar_quadrado_3x3(uint8_t r, uint8_t g, uint8_t b)
{
    // Limpa o buffer de LEDs
    for (int i = 0; i < NUM_LEDS; i++)
    {
        buffer_leds[i] = 0;
    }

    // Define os LEDs do quadrado 3x3 no centro da matriz 5x5
    int leds_quadrado[] = {6, 7, 8, 11, 12, 13, 16, 17, 18};
    for (int i = 0; i < 9; i++)
    {
        buffer_leds[leds_quadrado[i]] = converter_rgb_para_grb(r, g, b);
    }

    // Envia os pixels para a matriz de LEDs
    for (int i = 0; i < NUM_LEDS; i++)
    {
        enviar_pixel(buffer_leds[i]);
    }
}



// Função de tratamento de interrupções dos botões
void irq_handler(uint gpio, uint32_t events)
{
    static absolute_time_t last_interrupt_time = {0};
    absolute_time_t now = get_absolute_time();

    // Debounce: Responsável por ignorar interrupções muito próximas no tempo, evitando inúmeras leituras após o clique dos botões
    if (absolute_time_diff_us(last_interrupt_time, now) < 200000)
        return;
    last_interrupt_time = now;

    if (gpio == Botao_A)
    {
        coleta_dados = true; // Primeiro levanta a flag

        // Responsável por armazenar o contador de falhas (histórico) no vetor circular
        historico_falhas[pos_atual] = contador_falhas;
        pos_atual = (pos_atual + 1) % MAX_ENTRADAS;

        contador_falhas = 0; // Só zera após salvar
    }
    else if (gpio == Botao_B)
    {
        printf("Modo bootsel ativado");
        reset_usb_boot(0, 0); // Modo BOOTSEL
    }
}

// Função para definir as zonas de falha ao longo do eixo x do Joystick
bool zona_defeituosa(int valores_x)
{
    if ((valores_x >= 500 && valores_x <= 700) || (valores_x >= 3200 && valores_x <= 3500)) // zonas as quais indicam fissuras no Transformador
        return true;                                                                        // retorna um valor verdadeiro

    return false; // retorna um valor falso
}

// Função que verifica as condições do sistema, se há ou não falha identificada, realizando os alarmes visuais e sonoros se falhas aparecerem
void verifica_zona_defeituosa(int valores_x)
{
    if (zona_defeituosa(valores_x))
    {
        if (!estava_em_falha)
        {
            contador_falhas++;      // só incrementa se não estava
            estava_em_falha = true; // Invertendo o valor para verdadeiro
            printf("\nFissura detectada no transformador");
        }
        gpio_put(LED_R, true);        // LED vermelho aceso
        for (int i = 0; i < 100; i++) // 100 ciclos para o Buzzer
        {
            gpio_put(BUZZER, true);
            sleep_us(1000); // 1000 µs = frequência de 1 kHz - melhor sonoridade
            gpio_put(BUZZER, false);
            sleep_us(1000);
            gpio_put(LED_G, false);
        }
    }
    else
    {
        estava_em_falha = false; // trazendo variável para valor falso novamente
        gpio_put(LED_G, true);   // Condição de material sem defeito, led verde on
        gpio_put(LED_R, false);  // LED vermelho apagado
        gpio_put(BUZZER, false); // Buzzer desligado
    }
}

int main()
{
    stdio_init_all();

    // Configuração do botão A
    gpio_init(Botao_A);
    gpio_set_dir(Botao_A, GPIO_IN);
    gpio_pull_up(Botao_A);
    gpio_set_irq_enabled_with_callback(Botao_A, GPIO_IRQ_EDGE_FALL, true, &irq_handler);

    // Configuração do botão B
    gpio_init(Botao_B);
    gpio_set_dir(Botao_B, GPIO_IN);
    gpio_pull_up(Botao_B);
    gpio_set_irq_enabled_with_callback(Botao_B, GPIO_IRQ_EDGE_FALL, true, &irq_handler);

    // Inicialização do LED Vermelho, deixando-o inicialmente off
    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_put(LED_R, false);

    // Inicialização do LED Verde, deixando-o inicialmente off
    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_put(LED_G, false);

    // Inicialização do Buzzer, deixando-o inicialmente off
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
    gpio_put(BUZZER, false);

    // Inicializa o I2C e o display SSD1306
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_DISPLAY, I2C_PORT);
    ssd1306_config(&ssd);

    // Inicializa o ADC para o joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    uint16_t adc_x, adc_y;       // Variáveis de análise do joystick
    int x_pos = 60, y_pos = 28;  // Posicionamento
    uint32_t tempo_ocorrido = 0; // variável 32 bits para realizar cálculo para contador_uart

    // Configuração da matriz de LEDs
    configurar_matriz_leds();
    sleep_ms(100);        // Pequeno atraso para garantir que a matriz esteja pronta
    limpar_matriz_leds(); // Garante que a matriz de LEDs comece desligada

    while (true)
    {

        // Leitura dos valores do joystick
        adc_select_input(1); // X
        adc_x = adc_read();
        adc_select_input(0); // Y
        adc_y = adc_read();

        // Verifica se adc_x está em uma zona defeituosa
        verifica_zona_defeituosa(adc_x);

        // Mapeamento dos valores ADC para coordenadas do quadrado
        y_pos = ((adc_x * (WIDTH - 8)) / 4095);               // Largura: 128
        x_pos = HEIGHT - 8 - ((adc_y * (HEIGHT - 8)) / 4095); // Altura: 64

        // Garante que o quadrado fique dentro dos limites do display
        if (y_pos < 0)
            y_pos = 0;
        if (y_pos > WIDTH - 8)
            y_pos = WIDTH - 8;
        if (x_pos < 0)
            x_pos = 0;
        if (x_pos > HEIGHT - 8)
            x_pos = HEIGHT - 8;

        // Atualiza o display
        ssd1306_fill(&ssd, false);                          // Limpa o display
        ssd1306_rect(&ssd, x_pos, y_pos, 8, 8, true, true); // Desenha o quadrado
        ssd1306_send_data(&ssd);                            // Envia a informação pro display

        uint64_t contador_uart = to_ms_since_boot(get_absolute_time()); // pega o tempo absoluto (real) do programa
        if (contador_uart - tempo_ocorrido >= 1000)
        {
            tempo_ocorrido = contador_uart;                // Define agora tempo_ocorrido sempre a cada 1000 ms ao valor de contador_uart
            printf("\033[2J\033[H");                       // Printf de comandos ANSI, neste caso para limpeza da tela após o tempo determinado
            printf("Sensor PEC realizando inspeção\n");    // Mostra que o programa encontra-se ativo e já funcional
            printf("Valor do ADC no eixo x: %d\n", adc_x); // Printf que verifica os valores do ADC
        }

        // Caso o Botão A for pressionado, esta condição é ativa
        if (coleta_dados == true)
        {
            if (total_entradas < MAX_ENTRADAS)
            {
                total_entradas++;
            }

            // Exibe o últimos valores armazenados, no máximo 4 últimas coletas realizadas
            int pos_ultima = (pos_atual + MAX_ENTRADAS - 1) % MAX_ENTRADAS;
            printf("\nBotão A selecionado\n");
            printf("Dados coletados: %d falha(s) detectada(s)\n", historico_falhas[pos_ultima]);

            printf("Último(s) %d registro(s) de coleta:\n", total_entradas);
            printf("-----------------------------\n");
            printf("| Nº | Falha(s) detectada(s)|\n");
            printf("-----------------------------\n");

            // Exibe o histórico de falhas na ordem correta de inserção
            // A posição mais antiga aparece primeiro, e a mais recente por último
            for (int i = 0; i < total_entradas; i++)
            {
                int index = (pos_atual + MAX_ENTRADAS - total_entradas + i) % MAX_ENTRADAS;
                printf("| %d  | %d                    |\n", i + 1, historico_falhas[index]);
            }
            printf("-----------------------------\n");

            desenhar_quadrado_3x3(0, 255, 0); // Matriz de leds é ativa para indicar coleta de dados
            coleta_dados = false;             // Reseta a flag
            sleep_ms(2000);                   // Delay para manter tabela de dados e matriz mostradas e ativos, respectivamente, por 2 segundos
        }
        else
            desenhar_quadrado_3x3(0, 0, 0);

        sleep_ms(20); // Pequeno delay para suavizar a movimentação
    }
}

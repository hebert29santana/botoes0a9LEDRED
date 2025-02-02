/*
    Tarefa 1 Aula Síncrona 27/01/2025
    Hebert Costa Vaz Santana
    Matrícula: TIC370101235

    Botão esquerdo incrementa números
    Botão direito decrementa números

    Led Vermelho fica piscando sempre
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "ws2818b.pio.h"
#include "pico/multicore.h"

// Configurações dos pinos
#define LED_PIN 7          // Pino da matriz de LEDs
#define RGB_RED_PIN 13     // Pino vermelho do LED RGB
#define BTN_A_PIN 5        // Pino do botão A
#define BTN_B_PIN 6        // Pino do botão B
#define LED_COUNT 25       // Total de LEDs na matriz
#define DEBOUNCE_DELAY 200 // Tempo de debounce em ms

// Estrutura para representar um pixel RGB
struct pixel_t
{
    uint8_t G, R, B;
};

// Variáveis globais
volatile int current_number = 0;
volatile uint32_t last_button_time = 0;
typedef struct pixel_t pixel_t;
pixel_t leds[LED_COUNT];
PIO np_pio = pio0;
uint sm;

// Matriz de LEDs 5x5
const uint8_t MATRIZ_LEDS[5][5] = {
    {24, 23, 22, 21, 20},
    {15, 16, 17, 18, 19},
    {14, 13, 12, 11, 10},
    {5, 6, 7, 8, 9},
    {4, 3, 2, 1, 0}};

// Padrões dos números (0-9) para a matriz 5x5
// 1 representa LED aceso, 0 representa LED apagado
const uint8_t NUMBER_PATTERNS[10][5][5] = {
    {// 0
     {0, 1, 1, 1, 0},
     {0, 1, 0, 1, 0},
     {0, 1, 0, 1, 0},
     {0, 1, 0, 1, 0},
     {0, 1, 1, 1, 0}},
    {// 1
     {0, 1, 1, 0, 0},
     {0, 0, 1, 0, 0},
     {0, 0, 1, 0, 0},
     {0, 0, 1, 0, 0},
     {0, 1, 1, 1, 0}},
    {// 2
     {0, 1, 1, 1, 0},
     {0, 0, 0, 1, 0},
     {0, 1, 1, 1, 0},
     {0, 1, 0, 0, 0},
     {0, 1, 1, 1, 0}},
    {// 3
     {0, 1, 1, 1, 0},
     {0, 0, 0, 1, 0},
     {0, 1, 1, 1, 0},
     {0, 0, 0, 1, 0},
     {0, 1, 1, 1, 0}},
    {// 4
     {0, 1, 0, 1, 0},
     {0, 1, 0, 1, 0},
     {0, 1, 1, 1, 0},
     {0, 0, 0, 1, 0},
     {0, 0, 0, 1, 0}},
    {// 5
     {0, 1, 1, 1, 0},
     {0, 1, 0, 0, 0},
     {0, 1, 1, 1, 0},
     {0, 0, 0, 1, 0},
     {0, 1, 1, 1, 0}},
    {// 6
     {0, 1, 0, 0, 0},
     {0, 1, 0, 0, 0},
     {0, 1, 1, 1, 0},
     {0, 1, 0, 1, 0},
     {0, 1, 1, 1, 0}},
    {// 7
     {0, 1, 1, 1, 0},
     {0, 0, 0, 1, 0},
     {0, 0, 0, 1, 0},
     {0, 0, 1, 0, 0},
     {0, 1, 0, 0, 0}},
    {// 8
     {0, 1, 1, 1, 0},
     {0, 1, 0, 1, 0},
     {0, 1, 1, 1, 0},
     {0, 1, 0, 1, 0},
     {0, 1, 1, 1, 0}},
    {// 9
     {0, 1, 1, 1, 0},
     {0, 1, 0, 1, 0},
     {0, 1, 1, 1, 0},
     {0, 0, 0, 1, 0},
     {0, 0, 0, 1, 0}},
};

// Funções de inicialização dos LEDs WS2812
void npInit(uint pin)
{
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;
    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0)
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    for (uint i = 0; i < LED_COUNT; ++i)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Aplica a intensidade (ajusta o valor de R, G e B para o nível de intensidade desejado)
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b, uint8_t intensity)
{
    leds[index].R = (r * intensity) / 255;
    leds[index].G = (g * intensity) / 255;
    leds[index].B = (b * intensity) / 255;
}

void npWrite()
{
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
}

void npClear()
{
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        npSetLED(i, 0, 0, 0, 0);
    }
    npWrite();
}

void displayNumber(int number)
{
    npClear();
    if (number < 0 || number > 9)
        return;

    for (int row = 0; row < 5; row++)
    {
        for (int col = 0; col < 5; col++)
        {
            if (NUMBER_PATTERNS[number][row][col])
            {
                uint8_t led_index = MATRIZ_LEDS[row][col];
                npSetLED(led_index, 0, 0, 255, 80);
                sleep_us(50); // Atraso para evitar problemas de temporização
            }
        }
    }
    npWrite();
}

// Manipuladores de interrupção para os botões
void gpio_callback(uint gpio, uint32_t events)
{

    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - last_button_time >= DEBOUNCE_DELAY)
    {
        if (gpio == BTN_A_PIN && current_number < 9)
        {
            current_number++;
        }
        else if (gpio == BTN_B_PIN && current_number > 0)
        {
            current_number--;
        }
        last_button_time = current_time;
    }
}

// Função para piscar LED vermelho
void blink_red_led()
{
    for (int i = 0; i < 5; i++)
    {                             // Piscará 5 vezes (aceso e apagado)
        gpio_put(RGB_RED_PIN, 1); // Acende o LED
        sleep_ms(100);            // Espera 100ms
        gpio_put(RGB_RED_PIN, 0); // Apaga o LED
        sleep_ms(100);            // Espera 100ms
    }
    sleep_ms(1000); // Espera 1 segundo com o LED apagado
}

void blink_thread()
{
    while (true)
    {
        blink_red_led();
    }
}

int main()
{
    // Inicialização do sistema
    stdio_init_all();

    // Configuração dos pinos GPIO
    gpio_init(BTN_A_PIN);
    gpio_init(BTN_B_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);
    gpio_pull_up(BTN_B_PIN);
    gpio_init(RGB_RED_PIN);
    gpio_set_dir(RGB_RED_PIN, GPIO_OUT);

    // Configuração das interrupções
    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(BTN_B_PIN, GPIO_IRQ_EDGE_FALL, true);

    // Inicialização da matriz de LEDs
    npInit(LED_PIN);

    multicore_launch_core1(blink_thread);

    // Loop principal
    while (1)
    {
        // Exibe o número na matriz de LEDs com base no valor de current_number
        displayNumber(current_number);
        // Lógica de controle do LED RGB (piscar)
        // blink_red_led(); // Pisca o LED RGB   
    }

    return 0;
}

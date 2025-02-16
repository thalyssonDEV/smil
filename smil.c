#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ws2812b_animation.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include <string.h>
#include <stdio.h>

// Definição dos pinos de hardware utilizados
#define TRIG_PIN 17           /**< Pino de trigger do sensor ultrassônico */
#define ECHO_PIN 16           /**< Pino de eco do sensor ultrassônico */
#define BUZZER_PIN 10         /**< Pino de controle do buzzer */
#define BUTTON_PIN 5          /**< Pino do botão de controle de funcionamento */
#define BUTTON_NIGHT_MODE 6   /**< Pino do botão de controle do modo noturno */
#define JOYSTICK_VRX 27       /**< Pino X do joystick */
#define JOYSTICK_VRY 26       /**< Pino Y do joystick */
#define JOYSTICK_SW 22        /**< Pino do botão do joystick */
#define SCREEN_WIDTH 128      /**< Largura da tela do display */
#define SCREEN_HEIGHT 64      /**< Altura da tela do display */
#define SCREEN_ADDRESS 0x3C   /**< Endereço I2C do display SSD1306 */
#define I2C_SDA 14            /**< Pino SDA para comunicação I2C */
#define I2C_SCL 15            /**< Pino SCL para comunicação I2C */

#define ALTURA_MAX_LIXEIRA 120 /**< Altura máxima da lixeira em centímetros */

// Definição das constantes de leitura do joystick
#define JOYSTICK_VRY_MAX 3500  /**< Valor máximo para o eixo Y do joystick */
#define JOYSTICK_VRY_MIN 500   /**< Valor mínimo para o eixo Y do joystick */

// Estrutura para representar o estado do sistema da lixeira
typedef struct {
    int brilho;               /**< Nível de brilho dos LEDs */
    bool funcionando;         /**< Flag de funcionamento do sistema */
    bool modoNoturnoAtivado;  /**< Flag do modo noturno ativado */
    float distancia;          /**< Distância medida pelo sensor ultrassônico */
    float ocupacao;           /**< Percentual de ocupação da lixeira */
} SistemaLixeira;


// Instância do Display SSD1306
ssd1306_t display; /**< Inicializa a instância do display OLED */


// Instância do sistema da lixeira com valores iniciais
SistemaLixeira sistema = {
    .brilho = 6,
    .funcionando = false,
    .modoNoturnoAtivado = true,
    .distancia = 0.0,
    .ocupacao = 0.
};


/**
 * @brief Função para controlar o pino TRIG do sensor ultrassônico.
 * @param estado O estado a ser configurado para o pino (HIGH ou LOW)
 */
void escreverTrigPin(bool estado) {
    gpio_put(TRIG_PIN, estado);  /**< Envia o estado para o pino TRIG */
}


/**
 * @brief Função para obter a distância medida pelo sensor ultrassônico.
 * 
 * A função emite um pulso de trigger e mede o tempo até receber o eco,
 * calculando assim a distância com base na fórmula da velocidade do som.
 * 
 * @return Distância medida em centímetros.
 */
float obterDistanciaCM() {
    escreverTrigPin(0);  
    sleep_us(2);               /**< Aguarda 2 microssegundos */
    escreverTrigPin(1);  
    sleep_us(10);              /**< Emite o pulso de 10 microssegundos */
    escreverTrigPin(0);  

    absolute_time_t inicio = get_absolute_time();
    while (!gpio_get(ECHO_PIN)) inicio = get_absolute_time();  /**< Aguarda início do eco */

    absolute_time_t fim = get_absolute_time();
    while (gpio_get(ECHO_PIN)) fim = get_absolute_time();  /**< Aguarda fim do eco */

    int64_t duracao = absolute_time_diff_us(inicio, fim);  /**< Calcula a duração do eco */
    
    return duracao / 58.0;  /**< Converte a duração em cm */
}


/**
 * @brief Função para calcular a média das distâncias coletadas em várias leituras.
 * 
 * A função realiza múltiplas leituras do sensor ultrassônico e calcula a média
 * para evitar valores flutuantes causados por pequenas variações do sensor.
 * 
 * @param qtdLeituras Número de leituras a serem feitas.
 * @return A distância média calculada.
 */
float obterDistanciaMedia(int qtdLeituras) {
    float soma = 0;
    for (int i = 0; i < qtdLeituras; i++) {
        soma += obterDistanciaCM();  
        sleep_ms(10);  /**< Espera 10 milissegundos entre as leituras */
    }
    return soma / qtdLeituras;  /**< Retorna a média das distâncias */
}


/**
 * @brief Função para calcular o percentual de ocupação da lixeira com base na distância.
 * 
 * A ocupação é calculada com base na distância medida pelo sensor, levando em conta
 * a altura máxima da lixeira.
 * 
 * @param distancia Distância medida pelo sensor ultrassônico.
 * @return Percentual de ocupação da lixeira.
 */
float calcularOcupacao(float distancia) {
    if (distancia > ALTURA_MAX_LIXEIRA) return 0.0;  /**< Se a distância for maior que a altura máxima, a lixeira está vazia */
    if (distancia < 0) return 100.0;  /**< Se a distância for negativa, a lixeira está cheia */
    
    return 100.0 * (1.0 - (distancia / ALTURA_MAX_LIXEIRA));  /**< Calcula a ocupação em percentual */
}


/**
 * @brief Função para emitir um alerta sonoro caso o modo noturno não esteja ativado.
 * 
 * O alerta é emitido através do buzzer, ativando-o por 5 milissegundos.
 */
void emitirAlertaSonoro() {
    if (!sistema.modoNoturnoAtivado) { /**< Executa se o modo noturno não estiver ativado */
        gpio_put(BUZZER_PIN, 1); 
        sleep_ms(5);  /**< Emite som por 5 milissegundos */
        gpio_put(BUZZER_PIN, 0);  
    }
}


/**
 * @brief Função para centralizar o texto na tela do display SSD1306.
 * 
 * O texto é centralizado na linha especificada com base na largura do texto e da tela.
 * 
 * @param texto O texto a ser exibido.
 * @param linha A linha onde o texto será exibido.
 */
void centralizarTexto(const char *texto, int linha) {
    int larguraTexto = strlen(texto) * 6;  /**< Largura do texto em pixels */
    int posX = (SCREEN_WIDTH - larguraTexto) / 2;  /**< Calcula a posição X para centralizar */
    ssd1306_draw_string(&display, posX, linha, 1, texto);  /**< Exibe o texto na tela */
}


/**
 * @brief Função para atualizar o display com as informações do sistema.
 * 
 * A função limpa o display e exibe informações sobre o funcionamento do sensor,
 * a ocupação da lixeira, a distância medida e o status do modo noturno.
 */
void atualizarDisplayStatus() {
    ssd1306_clear(&display);  /**< Limpa o display */

    if (sistema.funcionando) {  /**< Se o sistema está funcionando */
        centralizarTexto("SENSOR: Ativado", 0);  /**< Exibe que o sensor está ativado */

        char textoOcupacao[32];
        snprintf(textoOcupacao, sizeof(textoOcupacao), "Ocupacao: %.1f%%", sistema.ocupacao);  
        centralizarTexto(textoOcupacao, 16);  /**< Exibe a ocupação da lixeira */

        char textoDistancia[32];
        snprintf(textoDistancia, sizeof(textoDistancia), "Distancia: %.1f cm", sistema.distancia);  
        centralizarTexto(textoDistancia, 32);  /**< Exibe a distância medida pelo sensor */

        char modoTexto[32];
        snprintf(modoTexto, sizeof(modoTexto), "Modo Noturno: %s", sistema.modoNoturnoAtivado ? "Enable" : "Disable");
        centralizarTexto(modoTexto, 48);  /**< Exibe o status do modo noturno */
    } else {  
        centralizarTexto("SENSOR: Desativado", SCREEN_HEIGHT / 2 - 8);  /**< Exibe que o sensor está desativado */
    }
    
    ssd1306_show(&display);  /**< Atualiza o display com as informações */
}


/**
 * @brief Função para ajustar o brilho global dos LEDs.
 * 
 * A função altera o brilho dos LEDs de acordo com o valor fornecido, garantindo que o brilho
 * esteja dentro dos limites definidos (0 a 7).
 * 
 * @param ajuste O valor a ser somado ou subtraído ao brilho atual.
 */
void ajustarBrilho(int ajuste) {
    sistema.brilho += ajuste; /**< Modifica a intensidade do brilho de acordo com o valor do ajuste */ 

    if (sistema.brilho > 7) sistema.brilho = 7;  
    if (sistema.brilho < 0) sistema.brilho = 0; /**< Garante que o brilho não seja maior que 7 e menor que 0 */

    ws2812b_set_global_dimming(sistema.brilho);  /**< Altera a intensidade global dos leds */

    printf("Brilho Ajustado Para: %d\n", sistema.brilho);  
}


/**
 * @brief Função para verificar as leituras do joystick e ajustar o brilho.
 * 
 * A função lê o valor do eixo Y do joystick e ajusta o brilho de acordo com o valor
 * lido, permitindo ao usuário controlar a intensidade dos LEDs.
 */
void verificarJoystick() {
    adc_select_input(0);  
    uint16_t vry = adc_read();  /**< Lê o valor do eixo Y do joystick */

    if (vry > JOYSTICK_VRY_MAX) {  /**< Se o valor do eixo Y for maior que o máximo, diminui o brilho */
        ajustarBrilho(-1);  
        sleep_ms(150);  /**< Pequeno delay para evitar mudanças muito rápidas */
    } 
    else if (vry < JOYSTICK_VRY_MIN) {  /**< Se o valor do eixo Y for menor que o mínimo, aumenta o brilho */
        ajustarBrilho(1);  
        sleep_ms(150);
    }

    if (!gpio_get(JOYSTICK_SW)) {  /**< Se o botão do joystick for pressionado, reseta o brilho para o valor padrão */
        sistema.brilho = 6;  
        ws2812b_set_global_dimming(sistema.brilho);  
        printf("Brilho resetado para: %d\n", sistema.brilho);  
        sleep_ms(300);  /**< Pequeno delay para evitar múltiplas leituras */
    }
}


/**
 * @brief Função para controlar o status do modo noturno.
 * 
 * A função alterna o estado do modo noturno, ativando ou desativando o comportamento
 * que pode alterar a luminosidade do sistema ou de outros componentes.
 */
void controlarModoNoturno() {
    sistema.modoNoturnoAtivado = !sistema.modoNoturnoAtivado;  /**< Verifica se o botão de modo noturno foi pressionado */
    if (sistema.modoNoturnoAtivado) {
        printf("Modo Noturno Ativado");
    } else {
        printf("Modo Noturno Desativado");
    }
}


/**
 * @brief Função para inicializar os pinos de hardware no início da execução do código.
 * 
 * A função inicializa todos os pinos necessários para o funcionamento do software.
 */
void inicializarPinos() {
    gpio_init(TRIG_PIN); /**< Inicializa o pino TRIG do sensor ultrassônico */
    gpio_set_dir(TRIG_PIN, GPIO_OUT); /**< Define o pino TRIG como saída */
    gpio_put(TRIG_PIN, 0); /**< Garante que o pino TRIG inicie com valor 0 */

    gpio_init(ECHO_PIN); /**< Inicializa o pino ECHO do sensor ultrassônico */
    gpio_set_dir(ECHO_PIN, GPIO_IN); /**< Define o pino ECHO como entrada */

    gpio_init(BUZZER_PIN); /**< Inicializa o pino do buzzer */
    gpio_set_dir(BUZZER_PIN, GPIO_OUT); /**< Define o pino do buzzer como saída */
    gpio_put(BUZZER_PIN, 0); /**< Desliga o buzzer inicialmente */

    gpio_init(BUTTON_PIN); /**< Inicializa o pino do botão de ativação do sensor */
    gpio_set_dir(BUTTON_PIN, GPIO_IN); /**< Define o pino do botão como entrada */
    gpio_pull_up(BUTTON_PIN); /**< Ativa o resistor de pull-up interno */

    gpio_init(BUTTON_NIGHT_MODE); /**< Inicializa o pino do botão de modo noturno */
    gpio_set_dir(BUTTON_NIGHT_MODE, GPIO_IN); /**< Define o pino do modo noturno como entrada */
    gpio_pull_up(BUTTON_NIGHT_MODE); /**< Ativa o resistor de pull-up interno para o botão de modo noturno */

    adc_init(); /**< Inicializa o ADC (Conversor Analógico para Digital) */
    adc_gpio_init(JOYSTICK_VRY); /**< Inicializa o pino de controle vertical do joystick */
    gpio_init(JOYSTICK_SW); /**< Inicializa o pino do botão do joystick */
    gpio_set_dir(JOYSTICK_SW, GPIO_IN); /**< Define o pino do botão do joystick como entrada */
    gpio_pull_up(JOYSTICK_SW); /**< Ativa o resistor de pull-up interno para o botão do joystick */

    ws2812b_init(pio0, 7, 25); /**< Inicializa a matriz de LEDs WS2812B */
    ws2812b_set_global_dimming(sistema.brilho); /**< Ajusta o brilho global dos LEDs */

    i2c_init(i2c1, 400 * 1000); /**< Inicializa a comunicação I2C com a frequência de 400kHz */
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); /**< Configura o pino SDA para função I2C */
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); /**< Configura o pino SCL para função I2C */
    gpio_pull_up(I2C_SDA); /**< Ativa o resistor de pull-up para o pino SDA */
    gpio_pull_up(I2C_SCL); /**< Ativa o resistor de pull-up para o pino SCL */
}


/**
 * @brief Função principal do sistema.
 * 
 * A função `main` inicializa o hardware, configura os pinos de entrada e saída, 
 * e gerencia a lógica de funcionamento do sistema. Ela controla o estado do sensor,
 * a ativação do modo noturno, o controle de brilho via joystick e a exibição dos dados 
 * no display, além de acionar o alarme sonoro quando necessário. A função executa 
 * continuamente enquanto o sistema está em operação.
 * 
 * @note Esta função entra em um loop contínuo, onde o estado do sistema é verificado
 *       e atualizado a cada iteração. 
 */
int main() {
    stdio_init_all(); /**< Inicializa a comunicação padrão */
    sleep_ms(2000);  /**< Aguarda 2 segundos para a inicialização completa */

    inicializarPinos(); /**< Inicializa todos os pinos de hardware necessários para o funcionamento do sistema */

    // Inicializa o display SSD1306
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) {
        printf("Falha ao inicializar o display\n");
        return 1; /**< Retorna 1 caso a inicialização do display falhe */
    }

    // Flags de controle de estado dos botões e do sistema
    bool ultimoEstadoBotao = false; /**< Armazena o último estado do botão de ativação */
    bool botaoPressionado = false; /**< Flag indicando se o botão de ativação foi pressionado */
    
    bool ultimoEstadoBotaoModoNoturno = false; /**< Armazena o último estado do botão de modo noturno */
    bool botaoPressionadoModoNoturno = false; /**< Flag indicando se o botão de modo noturno foi pressionado */

    // Loop principal do sistema
    while (true) {
        bool estadoBotao = gpio_get(BUTTON_PIN); /**< Lê o estado atual do botão de ativação */

        // Verifica se o botão foi pressionado (transição de estado)
        if (estadoBotao && !ultimoEstadoBotao) botaoPressionado = true;
        ultimoEstadoBotao = estadoBotao;

        // Alterna o estado de funcionamento do sistema ao pressionar o botão
        if (botaoPressionado) {
            sistema.funcionando = !sistema.funcionando;
            botaoPressionado = false;
            printf("Funcionamento %s\n", sistema.funcionando ? "ligado" : "desligado");
        }

        bool estadoBotaoModoNoturno = gpio_get(BUTTON_NIGHT_MODE); /**< Lê o estado atual do botão de modo noturno */

        // Verifica se o botão de modo noturno foi pressionado (transição de estado)
        if (estadoBotaoModoNoturno && !ultimoEstadoBotaoModoNoturno) botaoPressionadoModoNoturno = true;
        ultimoEstadoBotaoModoNoturno = estadoBotaoModoNoturno;

        // Alterna o estado do modo noturno ao pressionar o botão
        if (botaoPressionadoModoNoturno) {
            controlarModoNoturno(); /**< Ativa ou desativa o modo noturno */
            botaoPressionadoModoNoturno = false;
        }

        // Atualiza o funcionamento do sistema
        if (sistema.funcionando) {
            sistema.distancia = obterDistanciaMedia(10); /**< Obtém a média das distâncias medidas */
            sistema.ocupacao = calcularOcupacao(sistema.distancia); /**< Calcula a ocupação da lixeira com base na distância */

            printf("Distância: %.2f cm | Ocupação: %.1f%%\n", sistema.distancia, sistema.ocupacao);
            atualizarDisplayStatus();  /**< Atualiza o status no display */

            // Ajusta a cor dos LEDs conforme a ocupação da lixeira
            if (sistema.ocupacao < 65.0) {
                ws2812b_fill_all(GRB_GREEN); /**< LEDs verdes indicam baixo nível de ocupação */
            } else if (sistema.ocupacao < 85.0) {
                ws2812b_fill_all(GRB_YELLOW); /**< LEDs amarelos indicam ocupação média */
            } else {
                ws2812b_fill_all(GRB_RED); /**< LEDs vermelhos indicam alta ocupação */
                emitirAlertaSonoro(); /**< Emite um alerta sonoro se a ocupação for alta */
            }
            ws2812b_render(); /**< Atualiza os LEDs com a cor definida */

        } else {
            atualizarDisplayStatus(); /**< Atualiza o display para mostrar que o sistema está desligado */
            ws2812b_fill_all(GRB_BLACK); /**< Desliga todos os LEDs */
            ws2812b_render(); /**< Atualiza os LEDs para refletir a cor apagada */
        }

        verificarJoystick(); /**< Verifica o estado do joystick e ajusta o brilho */
        sleep_ms(100);  /**< Atraso de 100ms para evitar consumo desnecessário da CPU */
    }
}
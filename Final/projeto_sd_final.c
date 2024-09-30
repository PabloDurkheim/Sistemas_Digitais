#include <util/delay.h>
#include <avr/eeprom.h>

const int EEPROM_ADDR_TEMP = 0x0000; // Novo endereço na EEPROM para armazenar temperatura
const int EEPROM_ADDR_UMID = EEPROM_ADDR_TEMP + sizeof(float); // Novo endereço na EEPROM para armazenar umidade (após a temperatura)

volatile bool buttonPressed = false;
volatile int modo = 1; // modo de configuração
volatile unsigned long lastModeChangeMillis = 0;

//código ajusatado do livro AVR pagina 140

#define F_CPU 16000000UL // define a frequência do microcontrolador - 16MHz

#include <avr/io.h>       // definições do componente especificado
#include <util/delay.h>   // biblioteca para o uso das rotinas de _delay_ms e _delay_us()
#include <avr/pgmspace.h> // para a gravação de dados na memória flash
#include <avr/interrupt.h>

// Definições de macros para o trabalho com bits
#define set_bit(y, bit) (y |= (1 << bit))    // coloca em 1 o bit x da variável Y
#define clr_bit(y, bit) (y &= ~(1 << bit))   // coloca em 0 o bit x da variável Y
#define tst_bit(y, bit) (y & (1 << bit))    // testa se o bit x da variável Y está em 1

#define DADOS_LCD PORTH  // 4 bits de dados do LCD no PORTH
#define CONTR_LCD PORTB  // PORT com os pinos de controle do LCD (pino R/W em 0).
#define E PB4            // pino de habilitação do LCD (enable)
#define RS PB5           // pino para informar se o dado é uma instrução ou caractere

// sinal de habilitação para o LCD
#define pulso_enable()                   \
  _delay_us(1);                          \
  set_bit(CONTR_LCD, E);                  \
  _delay_us(1);                          \
  clr_bit(CONTR_LCD, E);                  \
  _delay_us(45)

// cd é a intrunção
void cmd_LCD(unsigned char c, char cd, bool flag)
{
  if (cd == 0)
  {
    clr_bit(CONTR_LCD, RS); // RS = 0 --> instrução
  }
  else
  {
    set_bit(CONTR_LCD, RS); // RS = 1 --> caractere
  }

  //indica se é pra enviar apenas 4 bits ou 4 bits mais significativos e mais 4 bits menos significativo
  if (flag)
  {
    DADOS_LCD = (DADOS_LCD & 0x87) | (c >> 1); // carregando dados do mais significativo
    pulso_enable();
  }
  else
  {
    DADOS_LCD = (DADOS_LCD & 0x87) | (c >> 1); // carregando dados do mais significativo
    pulso_enable();
    DADOS_LCD = (DADOS_LCD & 0x87) | (c << 3); // carregando dados do menos significativo
    pulso_enable();
  }

  if ((cd == 0) && (c < 4)) // se for instrução de retorno ou limpeza espera LCD estar pronto
    _delay_ms(2);
}

void inic_LCD_4bits() // sequência ditada pelo fabricando do circuito integrado HD44780
{
  clr_bit(CONTR_LCD, RS); // RS em zero indicando que o dado para o LCD será uma instrução
  clr_bit(CONTR_LCD, E);  // pino de habilitação em zero
  _delay_ms(20);

  cmd_LCD(0x28, 0, false); // modo de configuração do LCD
  cmd_LCD(0x08, 0, true);  // ligando o LCD
  cmd_LCD(0x01, 0, false); // limpando o LCD
  cmd_LCD(0x0C, 0, false); // desativa o cursor 
}

void escreve_LCD(char *c)
{
  for (; *c != 0; c++)
    cmd_LCD(*c, 1, false);
}

void pular_linha()
{
  cmd_LCD(0xC0, 0, false);
}

void limpar_LCD()
{
  cmd_LCD(0x01, 0, false);
}

bool pegarTemp_e_Umd(float &temperatura, float &umidade)
{
  // Realiza a leitura de dados do DHT22
  //unsigned long dados = 0;
  int umidadeRaw = 0;
  int temperaturaRaw = 0;
  
  DDRD = 0b0000100; // coloca o pino 19 (PD2) como saída
  // Envia o sinal de início para o sensor
  PORTD &= ~(1 << PD2); // LOW
  _delay_ms(18);        // pelo menos 18ms para garantir que o DHT22 detecte o sinal de início
  PORTD |= (1 << PD2);  // HIGH
  _delay_us(30);        // aguarda a resposta do sensor

  // Muda para o modo de entrada para ler a resposta do sensor
  DDRD &= ~(1 << PD2); // INPUT
  //PORTD |= (1 << PD2); // Habilita pull-up

  // Lê o sinal de resposta do sensor
  int timeout = 10000; // Timeout para evitar bloqueios
  while (!(PIND & (1 << PD2)))
  {
    if (timeout-- <= 0)
    {
      Serial.println("Timeout aguardando resposta1");
      return false;
    }
  }

  timeout = 10000; // Resetar timeout
  while ((PIND & (1 << PD2)))
  {
    if (timeout-- <= 0)
    {
      Serial.println("Timeout aguardando resposta2");
      return false;
    }
  }

  // Lê 40 bits de dados
  for (int i = 0; i < 40; i++)
  {
    timeout = 10000; // Resetar timeout
    while (!(PIND & (1 << PD2)))
    {
      if (timeout-- <= 0)
      {
        Serial.println("Timeout aguardando dados");
        return false;
      }
    }

    unsigned long startTime = micros();
    timeout = 10000; // Resetar timeout
    while (PIND & (1 << PD2))
    {
      if (timeout-- <= 0)
      {
        Serial.println("Timeout aguardando dados");
        return false;
      }
    }
    unsigned long duracao = micros() - startTime;

    // Interpreta os bits de dados
    if (i < 16)
    {
      // Bits 0 a 15 contêm a parte inteira da umidade
      umidadeRaw |= (duracao > 40) ? (1UL << (15 - i)) : 0;
    }
    else if (i < 32)
    {
      // Bits 16 a 31 contêm a parte inteira da temperatura
      temperaturaRaw |= (duracao > 40) ? (1UL << (31 - i)) : 0;
    }
  }
  
  // Converte para valores reais
  umidade = umidadeRaw / 10.0;

  // Verifica se a temperatura é negativa e ajusta
  if (temperaturaRaw & 0x8000) //constante 0x8000 que representa um número com o bit mais significativo (MSB) definido como 1 e os outros bits como 0
  {
    temperaturaRaw = -(temperaturaRaw & 0x7FFF); //outra constante que é uma máscara sem o MSB
  }
  temperatura = temperaturaRaw / 10.0;

  return true;
}

// Função para ler o valor analógico do pino A0
int readAnalogValue(byte pin)
{
  // Selecionar o canal analógico (neste caso, PF0)
  //ADMUX é o registrador que controla o multiplexador analogico conversor
  ADMUX = (ADMUX & 0xF0) | (pin & 0x0F);
  // Iniciar a conversão analógica
  //registrador de controle e de status 
  ADCSRA |= (1 << ADSC);
  // Aguardar o término da conversão
  while (ADCSRA & (1 << ADSC));
  // Ler e retornar o valor analógico
  return ADC;
}

void setup()
{
  Serial.begin(9600);
  Serial.println("Ligou");
  DDRH = 0xFF; // PORTH como saída - dados do lcd
  DDRB = 0xFF; // PORTB como saída - controle do lcd

  DDRL = 0xFF; //portas do LED como saídas
  //48 -> PL1 Azul
  //46 -> PL3 Verde
  //44 -> PL5 Vermelho

  inic_LCD_4bits();
  // escreve_LCD("Teste");

  PORTE |= (1 << PE4); // ativa o pul-up interno para o botão

  // Configurar a interrupção externa INT4 (PE4) na borda de subida
  EICRB |= (1 << ISC41) | (1 << ISC40); // Configura para borda de subida
  EIMSK |= (1 << INT4);

  //EICRB registrador de controle de borda de interrupção externa
  //EIMSK registrador de controle de máscara de interrupção externa

  //qualquer coisa olhar na pag 183 do livro AVR

}

void loop()
{
  float temperatura, umidade;

  temperatura, umidade = pegarTemp_e_Umd(temperatura, umidade);

  //pegar valor do "sensor de gas" na porta A0 -> PF0
  int gas = readAnalogValue(PF0);

  if(gas <= 341){
    //acende o azul e apaga os outros
    PORTL |= (1 << PL1); 
    PORTL &= ~(1 << PL3);
    PORTL &= ~(1 << PL5);
  }else if(gas > 341 & gas <= 682){
    //acende o verde a apaga os outros
    PORTL &= ~(1 << PL1); 
    PORTL |= (1 << PL3);
    PORTL &= ~(1 << PL5);
  }else if(gas > 682){
    //acende o vermelho e apaga os outros
    PORTL &= ~(1 << PL1); 
    PORTL &= ~(1 << PL3);
    PORTL |= (1 << PL5);
  }

  if (buttonPressed)
  {
    // Incrementa o modo e reinicia se necessário
    //modo = (modo % 3) + 1;
    buttonPressed = false; // Reinicia a sinalização
    lastModeChangeMillis = millis(); // Registra o tempo do último pressionamento do botão
    
    Serial.print("Modo: ");
    Serial.println(modo);
    
    atualizarDisplay(temperatura, umidade);

  }

  // Atualiza o display de acordo com o modo
  unsigned long currentMillis = millis();
  unsigned long interval = 0;

  switch (modo)
  {
  case 1:
    interval = 1000;
    break;
  case 2:
    interval = 5000;
    break;
  case 3:
    interval = 10000;
    break;
  default:
    break;
  }

  if (currentMillis - lastModeChangeMillis >= interval)
  {
    if (pegarTemp_e_Umd(temperatura, umidade))
    {
      atualizarDisplay(temperatura, umidade);
    }
    lastModeChangeMillis = currentMillis; // Reinicia o temporizador
  }

}

// Função para atualizar o display
void atualizarDisplay(float temperatura, float umidade)
{
  // Converter float para string
  char tempStr[10], umidStr[10];
  dtostrf(temperatura, 4, 1, tempStr); // 4 é o número total de caracteres, 1 é o número de casas decimais
  dtostrf(umidade, 4, 1, umidStr);

  // Limpar e escrever no LCD
  limpar_LCD();

  if(modo == 1) escreve_LCD("Modo: 1");
  if(modo == 2) escreve_LCD("Modo: 2");
  if(modo == 3) escreve_LCD("Modo: 3");
  pular_linha();
  escreve_LCD("T:");
  escreve_LCD(tempStr);
  escreve_LCD("C ");
  //pular_linha();
  escreve_LCD("U:");
  escreve_LCD(umidStr);
  escreve_LCD("%");

  EEPROM_escrita(EEPROM_ADDR_TEMP, temperatura);
  EEPROM_escrita(EEPROM_ADDR_UMID, umidade); 

  Serial.print("Lendo Temperatura salva: ");
  Serial.println(EEPROM_leitura(EEPROM_ADDR_TEMP));
  Serial.print("Lendo umidade salva: ");
  Serial.println(EEPROM_leitura(0x0004));
  Serial.println();
  //Serial.println( tempStr);

}

// Função de interrupção externa para o botão (pino 2, PE4)
ISR(INT4_vect)
{
  // Aguarda um curto período de debounce
  _delay_ms(50);

  // Verifica se o botão está realmente pressionado
  if ((PINE & (1 << PE4)) && !buttonPressed)
  {
    buttonPressed = true;
    modo++;

    if (modo > 3)
    {
      modo = 1;
    }
    //Serial.println(modo);
  }
}

//Código base do livro AVR pag. 173

// Função para escrever na EEPROM
void EEPROM_escrita(unsigned int uiEndereco, float ucDado) {
  while (EECR & (1 << EEPE)); // espera completar uma escrita prévia  
  //EEPE (EE Prom Write Enable) 
  EEAR = uiEndereco; // carrega o endereço para a escrita
  EEDR = ucDado; // carrega o dado a ser escrito
  //Serial.println(ucDado);
  EECR |= (1 << EEMPE); // escreve um lógico em EEMPE
  //EEMPE (EEPROM Master Write Enable),
  EECR |= (1 << EEPE); // inicia a escrita ativando EEPE
}

// Função para ler da EEPROM
unsigned char EEPROM_leitura(unsigned int uiEndereco) {
  while (EECR & (1 << EEPE)); // espera completar uma escrita prévia
  EEAR = uiEndereco; // escreve o endereço de leitura
  EECR |= (1 << EERE); // inicia a leitura ativando EERE
  return EEDR; // retorna o valor lido do registrador de dados
}

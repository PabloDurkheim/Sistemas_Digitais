#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <LiquidCrystal.h>
#include "HX711.h"

LiquidCrystal lcd(13, 12, 5, 4, 14, 15);
volatile int mode = 0;
volatile float abast_racao = 500;
volatile int porcao = 1;
volatile bool manual = 0;
volatile unsigned long lastDebounceTime = 0;
volatile unsigned long debounceDelay = 50;
volatile unsigned long servoChangeTime = 0;
volatile unsigned long potBelowThresholdTime = 0;
const unsigned long servoChangeDelay = 10000;  // 10 segundos
const unsigned long potBelowThresholdDelay = 10000; // 10 segundos

const int LOADCELL_DOUT_PIN = 21;
const int LOADCELL_SCK_PIN = 20;
long peso = 0;
long novoPeso = 0;
long refZero = -1116;
long variacao = 54; //para 168g
long extraZero = 0;
int pegarVar = 0;

HX711 scale;

void sendMessage(char data) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = data;
}

void ADC_Init() {
    // Configura o conversor analógico-digital (ADC)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);  // Habilita o ADC, ajusta o fator de divisão de clock para 64
    ADMUX = (1 << REFS0);  // Define a referência de tensão para AVCC
}

uint16_t ADC_Read(uint8_t channel) {
    // Realiza a leitura do canal especificado
    ADMUX = (ADMUX & 0xF8) | (channel & 0x07);
    ADCSRA |= (1 << ADSC);  // Inicia a conversão
    while (ADCSRA & (1 << ADSC));  // Aguarda a conversão ser concluída
    return ADC;
}

void setupInterrupts() {
    EICRA = 0b00000011;
    EIMSK |= (1 << INT3);
    EIMSK |= (1 << INT2);

    sei();
}

// Interrupção p/ armazenar aferição
ISR(INT3_vect) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
        mode++;
        if (mode > 2) {
            mode = 0;
        }
        while (!(PIND & (1 << PIND3))) {
            delay(100);
        }
        lastDebounceTime = millis();
    }
}

// Interrupção p/ armazenar aferição
ISR(INT2_vect) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (mode == 0) {
          manual=1;
        } else if (mode == 1) {
            abast_racao = abast_racao + 50;
        } else if (mode == 2) {
            porcao = porcao + 1;
            if (porcao > 5) {
                porcao = 1;
            }
        }

        if (abast_racao > 1000) {
            abast_racao = 0;
        }
        while (!(PIND & (1 << PIND2))) {
            delay(100);
        }
        lastDebounceTime = millis();
    }
}


int main(){
  init();
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  sei();
  lcd.begin(16, 2);
  lcd.clear();
  DDRB = 0b11100000;  // Configura o pino 11 como saída para o servo
  PORTB = 0b00011111;

  PORTD |= (1 << PD3); // pullup botao pin 2
  PORTD |= (1 << PD2); // pullup botao pin 2 
  UBRR0 = 103; 
  UCSR0A = 0;
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
  ADC_Init();
  setupInterrupts();
  
    while (1) {
        // Leitura do valor do potenciômetro no pino A0 (canal 0)
        //uint16_t pot_value = ADC_Read(0);

         if (scale.is_ready()) {
    
        //long peso = scale.read()/1000;
        peso = scale.read_average(5)/1000;
        //variacao = peso - refZero;
        //long x = peso - (-1141400);

        if(pegarVar == 0){

          extraZero = (168*(peso - refZero) / variacao);    

          if(peso > refZero){
            extraZero = extraZero*-1;
          }
      
          //variacao = variacao + extraZero;
    
          pegarVar = 1;

          }

        novoPeso = (168*(peso - refZero) / variacao) + extraZero;

        if(peso <= -1115 || novoPeso < 0){
          novoPeso = 0;
        } 
        
      }  

        // Defina a posição do servo com base no valor do potenciômetro
        if (novoPeso > (abast_racao)) {
            // OCR1A = 5000;  // Posição 2 (por exemplo, 180 graus)
            sendMessage('2');
            potBelowThresholdTime = 0; // reinicia o contador quando potenciômetro está acima de 10
        } else if (novoPeso < 10) {
            if (potBelowThresholdTime == 0) {
                potBelowThresholdTime = millis();
            } else if ((millis() - potBelowThresholdTime > potBelowThresholdDelay)||manual==1) {
                // Servo só se move após 10 segundos abaixo de 10
                sendMessage('1');
                servoChangeTime = millis();
                manual=0;
                potBelowThresholdTime = 0; // reinicia o contador
            } 
        }else{
              potBelowThresholdTime = 0; // reinicia o contador quando potenciômetro está acima de 10
        } 
 
        if (mode == 0) {
            lcd.setCursor(0, 0);
            lcd.print("Quant. de racao:");
            lcd.setCursor(0, 1);
            lcd.print(int(novoPeso));
            lcd.print("g                 ");
        } else if (mode == 1) {
            lcd.setCursor(0, 0);
            lcd.print("Selecione a qnt.");
            lcd.setCursor(0, 1);
            lcd.print("-    ");
            lcd.print(abast_racao);
            lcd.print("g   +");
        } else if (mode == 2) {
            lcd.setCursor(0, 0);
            lcd.print("Porcoes diarias:       ");
            lcd.setCursor(0, 1);
            lcd.print("-   ");
            lcd.print(porcao);
            if (porcao == 1) {
                lcd.print(" porcao   +");
            } else {
                lcd.print(" porcoes  +");
            }
        }

        _delay_ms(100);  // Pequeno atraso
    }
}

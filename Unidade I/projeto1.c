#include <LiquidCrystal.h>
#include <dht.h>

#define DHT22_PIN 9
#define potenciometroPin A0 //PF0
#define buzzerPin PJ0

dht DHT;

LiquidCrystal lcd(12, 11, 6, 5, 4, 7);

const byte buttonPinBit = 4; // PE4
const int debounceDelay = 50;
//byte buttonState = LOW;
//byte lastButtonState = LOW;
int counter = 1;
unsigned long lastDebounceTime = 0;
unsigned long lastDisplayUpdateTime = 0;
unsigned long interval = 1000;

volatile bool buttonPressed = false; // Variável para indicar que o botão foi pressionado

void setup() {

  //DDRE &= ~(1 << buttonPinBit);
  PORTE |= (1 << buttonPinBit); //ativa o pul-up interno para o botão

  // Configurar a interrupção externa INT4 (PE4) na borda de subida
  EICRB &= ~(1 << ISC41);
  EIMSK |= (1 << INT4);

  DDRJ |= (1 << DDJ0); // define o buzzer como saida

  // Configurar o pino A0 (potenciômetro) como entrada -> não precisa
  //DDRF &= ~(1 << PF0);
  // Ativar o resistor de pull-up interno no pino A0 (potenciômetro)
  //PORTF |= (1 << PF0); 

  Serial.begin(9600);

  lcd.begin(16, 2);

  //int chk = DHT.read22(DHT22_PIN);

  displayMode(counter);
}

void loop() {
  unsigned long currentMillis = millis();
  //Serial.println(currentMillis);
  if (buttonPressed) {
    buttonPressed = false;

    counter++;
    if (counter > 3) {
      counter = 1;
    }

    // Atualiza o intervalo de acordo com o modo
    switch (counter) {
      case 1:
        interval = 1000; // Modo 1: 1 segundo
        break;
      case 2:
        interval = 3000; // Modo 2: 3 segundos
        break;
      case 3:
        interval = 5000; // Modo 3: 5 segundos
        break;
    }

    displayMode(counter);
  }

  // Atualiza o display de acordo com o temporizador
  if (currentMillis - lastDisplayUpdateTime >= interval) {
    lastDisplayUpdateTime = currentMillis;
    displayMode(counter);
  }

  // Ler o valor analógico do pino A0 (potenciômetro)
  int potValue = readAnalogValue(PF0);
  //Serial.println(potValue);
  if (potValue > 400) { // limite apropriado
    tone(15, 262, 250);
  }

  // Resto do seu código loop
}

// Função para ler o valor analógico do pino A0
int readAnalogValue(byte pin) {
  // Selecionar o canal analógico (neste caso, PF0)
  ADMUX = (ADMUX & 0xF0) | (pin & 0x0F);
  // Iniciar a conversão analógica
  ADCSRA |= (1 << ADSC);
  // Aguardar o término da conversão
  while (ADCSRA & (1 << ADSC));
  // Ler e retornar o valor analógico
  return ADC;
}

void displayMode(int mode) {
  //(PORTB &=~(1<< PB6));
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Modo: ");

  switch (mode) {
    case 1:
      lcd.print("1 ");
      break;
    case 2:
      lcd.print("2 ");
      break;
    case 3:
      lcd.print("3 ");
      break;
  }

  //não mexer para baixo nivel, pois no lab vamos usar o DHT11 na hora de montar
  int chk = DHT.read22(DHT22_PIN);

  //lcd.setCursor(0, 1);
  lcd.command(0x80 | (0 + 1 * 0x40));
  lcd.print("T:");
  lcd.print(DHT.temperature, 1);
  lcd.print("C");

  //lcd.setCursor(9, 1);
  lcd.command(0x80 | (8 + 1 * 0x40));
  lcd.print("U:");
  lcd.print(DHT.humidity, 1);
  lcd.print("%");
}

// Função de interrupção externa para o botão (pino 2, PE4)
ISR(INT4_vect) {
  unsigned long currentMillis = millis();
  if (currentMillis - lastDebounceTime > debounceDelay) {
    if ( !(PINE & (1 << PE4 )) ){ // Verifica se o botão está pressionado
      buttonPressed = true;
      lastDisplayUpdateTime = currentMillis; // Atualize imediatamente o display ao pressionar o botão
    }
    lastDebounceTime = currentMillis;
  }
}

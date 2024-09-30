#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 16000000UL
#define set_bit(address, bit) (address |= (1 << bit))
#define clr_bit(address, bit) (address &= ~(1 << bit))
#define TOP 39999

void configServo(){
    DDRB = 0b00000010;
    ICR1 = TOP;

    TCCR1A = (1 << WGM11);
    TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);
    set_bit(TCCR1A, COM1A1);

    OCR1A = 1000;  // Variável para controlar a posição do servo
}

int main(){
  sei();
  UBRR0 = 103;
  UCSR0A = 0;
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
  DDRB = 0b00000111;
  configServo();
  
  while(1){
  }
}

ISR(USART_RX_vect) {
  char data = UDR0;
  switch (data) {
    case '1':
      OCR1A = 8000; 
      break;
    case '2':
      OCR1A = 1000; 
      break;
  }

  while (!(UCSR0A & (1 << UDRE0))); 
  UDR0 = data;
}

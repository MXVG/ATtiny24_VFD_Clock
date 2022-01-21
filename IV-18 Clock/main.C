#include <macros.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>



//Pins
#define LOAD        A,2
#define CLK         A,1
#define DATA        A,0
#define BLANK       A,5 

#define BTN1            A,3 //HRS
#define BTN2            A,4 //MINUTES

#define F_CPU 16000000UL // 16 MHz

// T1
volatile int ticks = 0;
volatile int seconds = 0;
volatile int minutes = 11;
volatile int hours = 11;

//OCR0A register value
volatile int reg_0A = 124;


// buttons
unsigned char btn1; //h
unsigned char btn2; //m

// update display flag
unsigned char displayUpdate = 1;

int currDigit = 8; 

// 9 bits for the digits (8 + sign)
unsigned int digits[] =
{
  1, //right
  2,
  4,
  8,
  16,
  32,
  64,
  128,
  256, //left
};



const unsigned char font[] =
{

  238, // 0
  36, // 1
  186, // 2
  182, // 3
  116, // 4
  214, // 5
  222, // 6
  164, // 7
  254, // 8
  244, // 9
  16, // -
  188, // h
  252, // A
  94, // b
  0, //NA
};



int buff[] = {1, 1, 1, 1, 1, 1, 1, 1, 128};


void initOutput()
{
  drive(CLK);
  drive(DATA);
  drive(LOAD);
  drive(BLANK);
  
  tristate(BTN1);
  tristate(BTN2);
  
}


/* VFD_CLOCK
   clocks in a bit into shift register
*/
void reg_clk()
{
  set_output(CLK);
  
  clr_output(CLK);
  
}

/* VFD_WRITE
   clocks in bits for one char/num on one digit in the shift register
   sets all other shift register outputs to 0
*/
void vfd_write(unsigned int digit, unsigned char value)
{
  int i;

  clr_output(LOAD);


  // here comes the 8 digits + am/pm indicator
  for (i = 8; i >= 0 ; i--)
  {
    if  (digit & (1 << i) ) {
      set_output(DATA);
    }
    else {
      clr_output(DATA);
    }
    reg_clk();
  }


  // here comes the 8 segments
  for (i = 7; i >= 0 ; i--)
  {
    clr_output(BLANK);
    if  (value & (1 << i) ) {
      set_output(DATA);
    }
    else {
      clr_output(DATA);
    }
    reg_clk();
    set_output(BLANK);
  }

  reg_clk();

}

int main(void) {


  cli();
  
  
  //set clock prescaler to 1
  CLKPR = 0x00;
  CLKPR |= (1 << CLKPCE);
  //CLKPR |= (1 << CLKPS1) | (1 << CLKPS0); //| (0 << CLKPCE);
  CLKPR |= (0 << CLKPS3) | (0 << CLKPS2) | (0 << CLKPS1) | (0 << CLKPS0);
  

  //TIMER0
  //Time keeping
  TCCR0A = 0x00;
  TCCR0B = 0x00;

  TCCR0B |= (1 << CS02) | (1 << CS00); // prescaler to 1024
  TCCR0A |= (1 << WGM01); // set to CTC mode

  OCR0A = reg_0A; //ISR at 124 (0.008ms)
  
  TCNT0 = 0;

  TIMSK0 |= (1 << OCIE0A); // enable interrupt on OCR0A match

  //TIMER1
  //Pwm boost converter @ ~32kHz
  TCCR1A = 0x43; //Toggle OCR1A pin
  TCCR1B = 0x19; //fast pwm mode clear on OCR1A

  OCR1A = 500;
  TCNT1 = 0;
  
  initOutput(); //Set pin modes
  
  sei();
  
  while (1) {

    vfd_write(digits[currDigit], font[buff[currDigit]]);
    currDigit--;


    if (get_input(BTN1) == 0)
    {
      btn1 = 1;
    }

    if (get_input(BTN2) == 0)
    {
      btn2 = 1;
    }


    if (get_input(BTN1) && btn1)
    {
      seconds = 0;
      hours++;
      if (hours == 12) {
        buff[8] ^= 1;
      }
      displayUpdate = 1;
      btn1 = 0;
    }

    if (get_input(BTN2) && btn2)
    {
      seconds = 0;
      minutes++;
      displayUpdate = 1;
      btn2 = 0;
    }


    if (currDigit < 0) {
      currDigit = 8;
    }

    if (displayUpdate != 0) {
      buff[7] = hours / 10;
      buff[6] = hours % 10;
      buff[5] = 10;
      buff[4] = minutes / 10;
      buff[3] = minutes % 10;
      buff[2] = 10;
      buff[1] = seconds / 10;
      buff[0] = seconds % 10;
      displayUpdate = 0;
    }


  }
}

//ISR for time keeping
ISR(TIM0_COMPA_vect) {
  
  cli();

  // 124 * 0.008ms = ~1 second
  if (ticks == reg_0A) {

    ticks = 0;
    

    seconds++;
    displayUpdate = 1;

    if (seconds > 59)
    {
      minutes++;
      seconds = 0;
      displayUpdate = 1;
    }

    if (minutes > 59)
    {
      hours++;
      minutes = 0;
      if (hours == 12) {
        buff[8] ^= 1;
      }
    }

    if (hours < 10 && digits[7] == 128)
    {
      digits[7] = 0;

    } else if (hours >= 10 && digits[7] == 0) {
      digits[7] = 128;
    }

    if (hours > 12)
    {
      hours = 1;
    }
    
  } else {
    ticks++;
  }
  sei();
}
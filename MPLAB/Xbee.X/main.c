#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include<stdlib.h>
#include "spi.h"
#include "font.h"
#include "ST7789.h"
#include <string.h>

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = FRCPLL // use internal oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = OFF // RC mode
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1048576 // use largest wdt
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

//#define VOLTS_PER_COUNT (3.3/1024)
char q[100];
char m[100];
//int RS = 500;
//int new_RS = 500; 
//int LED_RS = 500;

void initSPI();
void writeUART1(const char * string);
void readUART1(char * message, int maxLength);
unsigned int adc_sample_convert(int pin);

// Write interupt functions
void __ISR(_UART_1_VECTOR, IPL6SOFT) UART1RX_Handler(void){
    // Call the read UART
    readUART1(q,100); //call on readUART1 to send PIC a message
    
    writeUART1("50\r\n");
    
    // Turn off the flag so it can be triggered again
    IFS1bits.U1RXIF = 0;
    
}

int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISBbits.TRISB4 = 1; //B4 is input
    TRISAbits.TRISA4 = 0; //A4 is output
    LATAbits.LATA4 = 0; //A4 is low
    
    initSPI();
    LCD_init();
    
    // Set up the UART for XBee communication
    U1RXRbits.U1RXR = 0b0001; // U1RX is B6
    RPB7Rbits.RPB7R = 0b0001; // U1TX is B7
    
    // turn on UART1 with interrupt
    U1MODEbits.BRGH = 0; 
    U1BRG = ((48000000 / 230400) / 16) - 1; //((SYS_FREQ / DESIRED_BAUD)/16)-1

    // 8 bit, no parity bit, and 1 stop bit (8N1 setup)
    U1MODEbits.PDSEL = 0;
    U1MODEbits.STSEL = 0;
    
    IPC8bits.U1IP = 6; // Set the shadow register?
    IPC8bits.U1IS = 0; // Set the shadow register?
    
     IFS1bits.U1RXIF = 0; // Clear the flag to start
     IEC1bits.U1RXIE = 1; // These Enable Interupt

    // configure TX & RX pins as output & input pins
    U1STAbits.UTXEN = 1;
    U1STAbits.URXEN = 1;

    // enable the UART
    U1MODEbits.ON = 1;
    
    __builtin_enable_interrupts();

    while(1){
            
    }
}


// Read from UART1
// block other functions until you get a '\r' or '\n'
// send the pointer to your char array and the number of elements in the array
void readUART1(char * message, int maxLength) {
  char data = 0;
  int complete = 0, num_bytes = 0;
  
//   loop until you get a '\r' or '\n'
  while (!complete) {
    if (U1STAbits.URXDA) { // if data is available
      data = U1RXREG;      // read the data
      if ((data == '\n') || (data == '\r')) {
        complete = 1;
      }
      else {
        message[num_bytes] = data;
        ++num_bytes;
        // roll over if the array is too small
        if (num_bytes >= maxLength) {
          num_bytes = 0;
          
        }
      }
    }
  }
  // end the string
  message[num_bytes] = '\0';
}

// Write a character array using UART1
void writeUART1(const char * string) {
  while (*string != '\0') {
    while (U1STAbits.UTXBF) {
      ; // wait until TX buffer isn't full
    }
    U1TXREG = *string;
    ++string;
  }
}
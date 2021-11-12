
/* 
 * File:   main.c
 * Author: kailey
 *
 * Created on October 8, 2021, 7:45 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro

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
#pragma config POSCMOD = OFF // internal RC
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

void writeUART1(const char * string);
void readUART1(char * message, int maxLength);
unsigned int adc_sample_convert(int pin);

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
    
    // B4 is input for green LED
    TRISBbits.TRISB4 = 1; 
    // A4 is output for green LED
    TRISAbits.TRISA4 = 0; 
    // A4 is low
    LATAbits.LATA4 = 0; 
    
    // Also enables AN0?
    ANSELAbits.ANSA0 = 1;
    // Turn on the ADC
    AD1CON1bits.ADON = 1; 
    // Output the ACD data an a 16 bit integer
    AD1CON1bits.FORM = 0b000;
    //Assign AN0 to be an input
    TRISAbits.TRISA0 = 1; 
    
    // Set up the UART for XBee communication
    U1RXRbits.U1RXR = 0b0001; // U1RX is B6
    RPB7Rbits.RPB7R = 0b0001; // U1TX is B7
    
    // turn on UART1 without an interrupt
    U1MODEbits.BRGH = 0; 
    U1BRG = ((48000000 / 9600) / 16) - 1; //((SYS_FREQ / DESIRED_BAUD)/16)-1

    // 8 bit, no parity bit, and 1 stop bit (8N1 setup)
    U1MODEbits.PDSEL = 0;
    U1MODEbits.STSEL = 0;

    // configure TX & RX pins as output & input pins
    U1STAbits.UTXEN = 1;
    U1STAbits.URXEN = 1;

    // enable the UART
    U1MODEbits.ON = 1;
    
    // To assign reprogrammable pins, DONT USE TRIS AND LAT
    RPB15Rbits.RPB15R = 0b0101; // Set pin B15 to OC1
    RPB11Rbits.RPB11R = 0b0101; // Set pin B11 to OC2
    RPA2Rbits.RPA2R = 0b0101; // Set pin A2 to OC4
    RPA3Rbits.RPA3R = 0b0101; // Set pin A3 to OC3
    
    // Note for motor 1, IN1 = OC1 and IN2 = OC2
    // Coast: OC1 = 0 and OC2 = 0
    // Reverse: OC1 = 0 and OC2 = 1
    // Forward: OC1 = 1 and OC2 = 0
    // Brake: OC1 = 1 and OC2 = 1
    
    // Note for motor 2, IN1 = OC3 and IN2 = OC4
    // Coast: OC3 = 0 and OC4 = 0
    // Reverse: OC3 = 0 and OC4 = 1
    // Forward: OC3 = 1 and OC4 = 0
    // Brake: OC3 = 1 and OC4 = 1
    
    int RS; 
    RS = 800;
    
    T2CONbits.TCKPS = 2;     // set the timer prescaler so that you can use the largest PR2 value as possible without going over 65535 and the frequency is 50Hz
                             // possible values for TCKPS are 0 fo   r 1:1, 1 for 1:2, 2 for 1:4, 3 for 1:8, 4 for 1:16, 5 for 1:32, ...
    PR2 = 1999;              // max value for PR2 is 65535
    TMR2 = 0;                // initial TMR2 count is 0
    
    OC1CONbits.OCM = 0b110;    // PWM mode without fault pin; other OCxCON bits are defaults
    OC1RS = RS;               // duty cycle = OCxRS/(PR2+1)
    OC1R = RS;                // initialize before turning OCx on; afterward it is read-only
    
    OC2CONbits.OCM = 0b110;    // PWM mode without fault pin; other OCxCON bits are defaults
    OC2RS = RS;               // duty cycle = OCxRS/(PR2+1)
    OC2R = RS;                // initialize before turning OCx on; afterward it is read-only
    
    OC3CONbits.OCM = 0b110;    // PWM mode without fault pin; other OCxCON bits are defaults
    OC3RS = RS;               // duty cycle = OCxRS/(PR2+1)
    OC3R = RS;                // initialize before turning OCx on; afterward it is read-only
    
    OC4CONbits.OCM = 0b110;    // PWM mode without fault pin; other OCxCON bits are defaults
    OC4RS = RS;               // duty cycle = OCxRS/(PR2+1)
    OC4R = RS;                // initialize before turning OCx on; afterward it is read-only
    
    T2CONbits.ON = 1;        // turn on Timer2
    OC1CONbits.ON = 1;       // turn on OC1
    OC2CONbits.ON = 1;       // turn on OC2
    OC3CONbits.ON = 1;       // turn on OC3
    OC4CONbits.ON = 1;       // turn on OC4
    
    
    __builtin_enable_interrupts();

    char m[100];
    unsigned int adc;

    while (1) {
        OC2RS = RS;
        OC4RS = RS;
        
        if (PORTBbits.RB4 == 0){
            OC2RS = 0; 
            OC4RS = 0;
            int i;
            for (i = 0; i<2 ; i++){                  // run this block 4 times
                LATAINV = 0b10000;                  // toggle pin A4
                _CP0_SET_COUNT(0);                  // set sys clk to 0
                
                while(_CP0_GET_COUNT() < 0.5 * 24000000){;}  // delay 
           
            adc = adc_sample_convert(3);
//            sprintf(m, "Makis\r\n");
            sprintf(m, "ADC = %d\r\n", adc);
            writeUART1(m); //call on writeUART1 to send PIC a message
//            readUART1(m,100); //call on writeUART1 to send PIC a message
             
            }
        }
    }
}

// Read from UART1
// block other functions until you get a '\r' or '\n'
// send the pointer to your char array and the number of elements in the array
void readUART1(char * message, int maxLength) {
  char data = 0;
  int complete = 0, num_bytes = 0;
  // loop until you get a '\r' or '\n'
  while (!complete) {
    if (U1STAbits.URXDA) { // if data is available
      data = U1RXREG;      // read the data
      if ((data == '\n') || (data == '\r')) {
        complete = 1;
      } else {
        message[num_bytes] = data;
        ++num_bytes;
        // roll over if the array is too small
        if (num_bytes >= maxLength) {
          num_bytes = 0;
        }
      }
    }
  }
  LATAINV = 0b10000;                  // toggle pin A4
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

// Begins sampling from a specifed ANx pin, waits for the conversion to
// complete, and returns the result
unsigned int adc_sample_convert(int pin) {  // sample & convert the value on the given
                                            // adc pin the pin should be configured as an
                                            // analog input in AD1PCFG
    unsigned int elapsed = 0, finish_time = 0;

    AD1CON1bits.ON = 1;                     // ADC enabled
    AD1CON1bits.SSRC = 0b111;               // Auto Conversion
    AD1CON1bits.ASAM = 0;  

    AD1CHSbits.CH0SA = pin;                 // connect chosen pin to MUXA for sampling
    AD1CON1bits.SAMP = 1;                   // start sampling
    elapsed = _CP0_GET_COUNT();
    finish_time = elapsed + 10;
    while (_CP0_GET_COUNT() < finish_time) {
        ;                                       // sample for more than 250 ns
        }
    AD1CON1bits.SAMP = 0;                   // stop sampling and start converting
    while (!AD1CON1bits.DONE) {
        ;                                       // wait for the conversion process to finish
        }
    return ADC1BUF0;                        // read the buffer with the result
}
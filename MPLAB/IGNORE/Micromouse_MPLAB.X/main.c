#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include <stdio.h>
#include<stdlib.h>
#include "spi.h"
#include "font.h"
#include "Display_ST7789.h"
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

#define VOLTS_PER_COUNT (3.3/1024)
char q[100];
char m[100];
int RS = 500;
int new_RS = 500; 
int LED_RS = 500;

void initSPI();
void writeUART1(const char * string);
void readUART1(char * message, int maxLength);
unsigned int adc_sample_convert(int pin);

// Write interupt functions
void __ISR(_UART_1_VECTOR, IPL6SOFT) UART1RX_Handler(void){
    // Call the read UART
    LCD_clearScreen(0x0000);
    readUART1(q,100); //call on readUART1 to send PIC a message
    
    // Draw the UART message so I know it matches
//    drawString(28, 120, WHITE, "New PWM is..."); //FPS to screen
//    drawString(28, 140, WHITE, q); //FPS to screen
    
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
    
//    U1STA.UTXISEL = 0b00 // Interrupt is generated and asserted while the 
                         // transmit buffer contains at least one empty space
    
     //U1RX and U1TX Flag: IFS1, Enable: IEC1, Priority: IPC8, sub-pri: IPC8
     IFS1bits.U1RXIF = 0; // Clear the flag to start
//     IFS1bits.U1TXIF = 0 ; // Clear the flag to start
     IEC1bits.U1RXIE = 1; // These Enable Interupt
//     IEC1bits.U1TXIE = 0; // These Enable Interupt

    // configure TX & RX pins as output & input pins
    U1STAbits.UTXEN = 1;
    U1STAbits.URXEN = 1;

    // enable the UART
    U1MODEbits.ON = 1;
    
    // To assign reprogrammable pins, DONT USE TRIS AND LAT
    RPB15Rbits.RPB15R = 0b0101; // Set pin B15 to OC1
    RPB11Rbits.RPB11R = 0b0101; // Set pin B11 to OC2
    RPA3Rbits.RPA3R = 0b0101; // Set pin A3 to OC3
    RPA2Rbits.RPA2R = 0b0101; // Set pin A2 to OC4
    RPB2Rbits.RPB2R = 0b0110; // Set pin B2 to OC5
//    RPB13Rbits.RPB2R = 0b0110; // Set pin B13 to OC5
    
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
    
//    int RS, new_RS, LED_RS; 
//    
//    RS = 500;
//    new_RS = 500;
    LED_RS = 100;
    
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
    
    OC5CONbits.OCM = 0b110;    // PWM mode without fault pin; other OCxCON bits are defaults
    OC5RS = LED_RS;               // duty cycle = OCxRS/(PR2+1)
    OC5R = LED_RS;                // initialize before turning OCx on; afterward it is read-only
    
    T2CONbits.ON = 1;        // turn on Timer2
    OC1CONbits.ON = 1;       // turn on OC1
    OC2CONbits.ON = 1;       // turn on OC2
    OC3CONbits.ON = 1;       // turn on OC3
    OC4CONbits.ON = 1;       // turn on OC4
    OC5CONbits.ON = 1;       // turn on OC4
    
    __builtin_enable_interrupts();
    LCD_clearScreen(0x0000);
    
    unsigned int adc1, adc2;
    int i=0;
    char k[100];
//    char m[100];
//    char q[100];
    int num_HIs;
    num_HIs = 1;

    while(1){
        
        OC1RS = new_RS;
        OC2RS = new_RS;
        OC3RS = new_RS;
        OC4RS = new_RS;
        OC5RS = LED_RS;
        
        adc1 = adc_sample_convert(0b000);
        sprintf(k, "ADC1 = %5.1fV", adc1 * VOLTS_PER_COUNT);
//        drawString(28, 20, WHITE, k); //FPS to screen
        
        adc2 = adc_sample_convert(0b001);
        sprintf(k, "ADC2 = %5.1fV", adc2 * VOLTS_PER_COUNT);
//        drawString(28, 40, WHITE, k); //FPS to screen
        
        // If button is pushed
        if (PORTBbits.RB4 == 0){
//            OC2RS = 0; 
//            OC4RS = 0;
            
            LCD_clearScreen(BLACK); // Clear the damn screen so I know I'm storing stuff correctly
            
            int j;
            for (j = 0; j<2 ; j++){                  // run this block 4 times
                LATAINV = 0b10000;                  // toggle pin A4
                _CP0_SET_COUNT(0);                  // set sys clk to 0

                sprintf(m, "BUTTON PUSHED");
//                drawString(28, 60, WHITE, m); //FPS to screen

                // Only send the message once, even though we count twice for button
                // We wont exit this statement or the button / green LED until 
                // we write AND read from XBees
                if (j==1) {
                    LCD_clearScreen(BLACK); // Clear the damn screen so I know I'm storing stuff correctly
                    
                    // Make sure we're only sending and getting data once
                    sprintf(m, "Send message since j = %d", j);
//                    drawString(28, 100, WHITE, m); //FPS to screen

                    //Write out to the XBee attached to computer
                    //MAKE THIS A INTERUPT
                    sprintf(k, "Hi x%d\r\n", num_HIs);
                    writeUART1(k); //call on writeUART1 to send XBee a message
                    num_HIs = num_HIs + 1;
                }
                
                while(_CP0_GET_COUNT() < 1 * 24000000){;}  // delay

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
        if (data == 'w'){
//            drawString(28, 160, WHITE, "Move Forward"); //FPS to screen 
            OC1RS = RS;
            OC2RS = 0; 
            OC2RS = RS;
            OC4RS = 0;
        }
        else if (data=='s'){
//            drawString(28, 160, WHITE, "Move Backward"); //FPS to screen 
            OC1RS = 0; 
            OC2RS = RS;
            OC3RS = 0;
            OC4RS = RS;
            
        }
        else if (data=='d'){
//            drawString(28, 160, WHITE, "Move Right"); //FPS to screen 
            //Check that this is correct
            OC1RS = RS;
            OC2RS = 0; 
            OC3RS = 0;
            OC4RS = RS;
            
        }
        else if (data=='a'){
//            drawString(28, 160, WHITE, "Move Left"); //FPS to screen 
            OC1RS = 0; 
            OC2RS = RS;
            OC3RS = RS;
            OC4RS = 0;
            
        }
        else if (data=='q'){
//            drawString(28, 160, WHITE, "STOP"); //FPS to screen 
            OC1RS = RS;
            OC2RS = RS;
            OC3RS = RS;
            OC4RS = RS;
            
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
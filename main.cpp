
/* 
 * File:   main.c
 * Author: Hennie Kotze firmware@autonomech.co.za
 *
 * Created on 31 March 2019, 1:38 AM
 */

/******************************************************************************/
/*                             PROJECT PINOUT                                 */
/******************************************************************************/
/*                             ATMega328P
 *                                ______
 *            RESET/PCINT14/PC6 =|01* 28|= PC5/PCINT13/SCL/ADC5
 *               RX/PCINT16/PD0 =|02  27|= PC4/PCINT12/SDA/ADC4
 *               TX/PCINT17/PD1 =|03  26|= PC3/PCINT11/ADC3
 *             INT0/PCINT18/PD2 =|04  25|= PC2/PCINT10/ADC2
 *                  PCINT19/PD3 =|05  24|= PC1/PCINT9/ADC1
 *                  PCINT20/PD4 =|06  23|= PC0/PCINT8/ADC0
 *                          Vcc =|07  22|= GND
 *                          GND =|08  21|= Aref
 *             XTAL1/PCINT6/PB6 =|09  20|= AVcc
 *             XTAL2/PCINT7/PB7 =|10  19|= PB5/PCINT5/SCK
 *             OC0B/PCINT21/PD5 =|11  18|= PB4/PCINT4/MISO
 *        OC0A/AIN0/PCINT22/PD6 =|12  17|= PB3/PCINT3/MOSI/OC2A/OC2
 *             AIN1/PCINT23/PD7 =|13  16|= PB2/PCINT2/SS/OC1B
 *                   PCINT0/PB0 =|14  15|= PB1/PCINT1/OC1A
 *                                ------
 * 
 *                                ______
 *                              =|01* 28|= ***(Free)
 *                     UART___/ =|02  27|= PIN_RST (PC4)
 *                            \ =|03  26|= PIN_CS (PC3)
 *                 (PD2) PIN_02 =|04  25|= PIN_RS (PC2)
 *                 (PD3) PIN_03 =|05  24|= PIN_WR (PC1)
 *                 (PD4) PIN_04 =|06  23|= PIN_RD (PC0)
 *                          Vcc =|07  22|= GND
 *                          GND =|08  21|= Aref
 *                      XTAL__/ =|09  20|= AVcc
 *                            \ =|10  19|= \
 *                 (PD5) PIN_05 =|11  18|= |--SPI
 *                 (PD6) PIN_06 =|12  17|= /
 *                 (PD7) PIN_07 =|13  16|= ***(Free) DIAGNOSTIC OUTPUT / POWER PIN
 *                 (PB0) PIN_00 =|14  15|= PIN_01 (PB1) 
 *                                ------
 * 
 * 
 */



/******************************************************************************/
/*                                   DEFS                                     */
/******************************************************************************/

#define F_CPU           16000000
#define BAUD            9600
#define MYUBRR          F_CPU/16/BAUD-1
#define MEMSIZE         2048
#define RAM_START       0x0100
#define RAM_END        0x08FF
#define TRUE            1
#define FALSE           0
#define ON              1
#define OFF             0
#define HIGH            1
#define LOW             0
#define NOP             asm("nop")
#define TX_BUF_LEN      8
#define RX_BUF_LEN      8

//---MACROS
#define CMD_EQ(A)           strcmp(A, rxbuf) == 0
#define CMD_EQN(A,N)        strncmp(A, rxbuf, N) == 0
#define ITOA(A)             itoa(A, txbuf, 10)
#define ITOA2(A)            itoa(A, txbuf, 2)
#define ITOA16(A)           itoa(A, txbuf, 16)

#define _readpin(a,b)       (a & b)
#define _setpin(a,b)        a |= b
#define _clearpin(a,b)      a &= ~(b)
#define _togglepin(a,b)     a ^= ba

//diagnostix
//#define PIN_DIAG    0x04
//#define PORT_DIAG   PORTB
#define PIN_POWER    0x04
#define PORT_POWER   PORTB

//
#define CHAR_COLS               0x08
#define CHAR_ROWS               0x06
#define CHAR_BIN_WITDH          0x28//40 pixels
#define CHAR_BIN_HEIGHT         0x50//80 pixels
#define CHAR_BBOX_WITDH         0x28//40 pixels
#define CHAR_BBOX_HEIGHT        0x50//80 pixels



/******************************************************************************/
/*                                 INCLUDES                                   */
/******************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>
#include "math.h"
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
//#include <util/setbaud.h>
//#include <util/twi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "virOS.h"
#include "ili9481_opcodes.h"
#include "font1.h"
#include "tft_graphics.h"
#include "fontworker.h"
#include "virOSFS_UI.h"

/******************************************************************************/
/*                                VARIABLES                                   */

/******************************************************************************/

uint32_t timer_counter;
uint8_t status_byte = 0xff;
volatile uint32_t a_number = 0;
volatile uint32_t b_number = 0;

uint16_t white, black, red, green, blue;

volatile bool flipflop = true;
volatile bool update = true;




/******************************************************************************/
/*                             STRUCTS AND ENUMS                              */

/******************************************************************************/


/******************************************************************************/
/*                            FUNCTION DECLARATIONS                           */
/******************************************************************************/

int main(void);
static void init(void);
void mainloop(void);
//
void init_tftlcd(void);
void write_command(uint8_t);
void write_command(uint8_t cmd, uint8_t dat);
void write_command(uint8_t cmd, uint8_t dat[], uint8_t size);
void write_data(uint8_t dat[], uint8_t size);
void put_bus_byte(uint8_t);
uint8_t read_command(uint8_t);
uint8_t get_bus_byte(void);

//virOS
extern uint8_t install_virOSFS();
extern void EEPROM_read_bytes(ui16 address, uint8_t * buffer, uint8_t size);

//timer
ISR(TIMER0_OVF_vect);
//util
uint8_t reverse_bits(uint8_t);
void delay_n_us(uint16_t);
void delay_n_ms(uint16_t);
int available_sram(void);

/******************************************************************************
 *                                FUNCTIONS                                   *
 ******************************************************************************/

int main(void)
{


    white = rgb_565(31, 63, 31);
    black = rgb_565(0, 0, 0);
    red = rgb_565(31, 0, 0);
    green = rgb_565(0, 63, 0);
    blue = rgb_565(0, 0, 31);

    init();
    _clearpin(PORT_POWER, PIN_POWER);
    _delay_ms(250);
    _setpin(PORT_POWER, PIN_POWER);
    _delay_ms(250);
    init_tftlcd();
    initFS(2, //size
           5, //font char px w
           7, //font char px h
           1, //font char pad h
           2, //font char pad v
           0, //xpos
           0, //ypos
           white);

    //hello!
    print_str(get_virOS_message(intro_1));
    print_str(get_virOS_message(intro_2));
    //----install virOS
    if (install_virOSFS() == 0) print_str(get_pgm_str(virOS_FS_INSTALL_COMPLETE));
    else printfi_str(get_pgm_str(intro_3), EEPROM_read_byte(0));

    //mem
    ui16 imem_avail, imem_used;
    imem_avail = available_sram();
    imem_used = MEMSIZE - imem_avail;
    printfi_str(get_pgm_str(mem_used), imem_used);
    printfi_str(get_pgm_str(mem_free), imem_avail);


    mainloop();
    return 0;
}

/*******************************************************************************
 *                                                                        INIT*/

static void init(void)
{

    //=======================================================================I/O
    //Direction registers port c 1=output 0=input
    /*If DDxn is written logic one, Pxn is configured as an output pin. 
     * If DDxn is written logic zero, Pxn is configured as an input pin.
     * If PORTxn is written logic one AND the pin is configured as an input pin (0), 
     * the pull-up resistor is activated.
     */
    DDRB |= PIN_00 | PIN_01 | PIN_POWER;
    PORTB |= 0xff;

    DDRC |= PIN_RST | PIN_CS | PIN_CMD | PIN_WR | PIN_RD;
    PORTC |= 0xff;

    DDRD |= PIN_02 | PIN_03 | PIN_04 | PIN_05 | PIN_06 | PIN_07;
    PORTD |= 0xff;

    //=======================================================================ADC

    //======================================================================UART

    //====================================================================TIMER0
    TCNT0 = 0x00;
    TCCR0B |= _BV(CS00); // | (1 << CS02);//prescaler
    TIMSK0 |= _BV(TOV0); //timer 0 overflow interrupt enable

    //=======================================================================I2C

    //====================================================================EEPROM


    //================================================================INTERRUPTS
    sei();
}

/******************************************************************************
 *                                                                    MAINLOOP*/
void mainloop(void)
{



    //ui8 ar[] = {0xab, 0xcd, 0xef, 0x12, 0x034, 0x56, 0x78, 0xAA};
    ui8 ar[] = {0x12, 0x034, 0x56, 0x78, 0xAA, 0x034, 0x56, 0x78, 0xAA};
    uint8_t dummy_filename = 0b1101011;
    ui8 res = _fw(dummy_filename, (ui8 *) & ar, sizeof (ar));
    if (res == 0)
        printfi_str(get_pgm_str(virOS_FS_UI_FILE_WRITTEN), sizeof (ar));
    else if (res == FILE_ERR_ALREADY_EXISTS)
        print_str(get_virOS_message(virOS_FEC_ALREADY_EXISTS));


    const ui8 arrsize = 20;
    ui8 arr[arrsize] = {0};
    ui16 addr = 0x00; //FILE_SECTOR_START_ADDR - 6; //
    EEPROM_read_bytes(addr, arr, arrsize);
    ui8 i, c = 0;
    for (i = 0; i < arrsize; i++)
    {
        if (c > 7)
        {
            c = 0;
            print_str("\n\0");
        }
        printf_str("%02X ", &arr[i], TYPE_UCHAR);
        pFS->position.x = pFS->cursor.x;

        c++;
    }
    print_str("\n\0");

    FC fc = _fr(dummy_filename);
    if (fc.size > 0)
    {

        printfi_str("size: %02X\n\0", fc.size);
        c = 0;
        for (i = 0; i < fc.size; i++)
        {
            if (c > 7)
            {
                c = 0;
                print_str("\n\0");
            }
            printf_str("%02X ", &fc.data[i], TYPE_UCHAR);
            pFS->position.x = pFS->cursor.x;
            c++;

        }
        print_str("\n\0");
    }
    else
    {
        print_str("NOK\n\0");
    }

    _ls();
    _ll();

    while (TRUE)
    {
        if (update == true)
        {
            flipflop = !flipflop;
            if (flipflop)
            {
                fill_rect(310, 470, 5, 5, black);
            }
            else
            {

                fill_rect(310, 470, 5, 5, 0x1C3F);
            }
            NOP;
            update = false;
        }
    }
    return;
}

/******************************************************************************
 *                                                                  TFT INIT */

void init_tftlcd()
{
    //init lcd
    _RESET;
    //---power setting 0xD0,3
    uint8_t pwr[] = {0x07, 0x42, 0x18};
    write_command(OPC_POWER_SETTING, pwr, 3);
    //---vcom 0xD1,3
    uint8_t vcom[] = {0x00, 0x07, 0x10};
    write_command(OPC_VCOM_CTRL, vcom, 3);
    //---power mode 0xD2,2
    uint8_t pwrset[] = {0x01, 0x02};
    write_command(OPC_POWER_SETTING_NORMAL_MODE, pwrset, 2);
    //---panel driving 0xC0,5
    uint8_t pnldrv[] = {0x10, 0x3B, 0x00, 0x02, 0x11};
    write_command(OPC_PANEL_DRIVING_SETTING, pnldrv, 5);
    //---frame rate 0xC5,1
    write_command(OPC_FRAME_RATE_INVERT_CTRL, 0x03);
    //---gamma 0xC8,12
    uint8_t gamma[] = {0x00, 0x32, 0x36, 0x45, 0x06, 0x16, 0x37, 0x75, 0x77, 0x54, 0x0C, 0x00};
    write_command(OPC_GAMMA_SETTING, gamma, 12);
    //---addr mode 0x36,1
    write_command(OPC_WRITE_ADDR_MODE, 0x0A);
    //---pixel format 0x3A,1
    write_command(OPC_WRITE_PIXEL_FMT, PIXEL_FMT_16BITS);
    //---normal mode 0x13
    write_command(OPC_ENTER_NORMAL_MODE); /**/
    //---wake up 0x11
    write_command(OPC_EXIT_SLEEP_MODE);
    _delay_ms(120);
    //---display ON 0x29
    write_command(OPC_SET_DISPL_ON);
    _delay_ms(120);
    //---exit invert mode 0x20
    write_command(OPC_ENTER_INVERT_MODE);
    clear_screen();
    return;
}

/******************************************************************************
 *                                             HOST <--> DEVICE COMMUNICATIONS*/
void write_command(uint8_t cmd)
{
    active_CS;
    active_CMD;
    put_bus_byte(cmd);
    _IDLE;
    return;
}

void write_command(uint8_t cmd, uint8_t dat)
{
    active_CS;
    active_CMD;
    put_bus_byte(cmd);
    idle_CMD;
    put_bus_byte(dat);
    _IDLE;
    return;
}

void write_command(uint8_t cmd, uint8_t dat[], uint8_t size)
{
    active_CS;
    active_CMD;
    put_bus_byte(cmd);
    idle_CMD;
    for (uint8_t i = 0; i < size; i++)
    {
        put_bus_byte(dat[i]);
    }
    _IDLE;
    return;
}

void write_data(uint8_t dat[], uint8_t size)
{
    active_CS;
    for (uint16_t len = 0; len < size; len++)
    {
        put_bus_byte(dat[len]);
    }
    _IDLE;
    return;
}

void put_bus_byte(uint8_t byte)
{
    //LCD DATA PIN.: 7  6  5  4  3  2  1  0
    //PORT PIN.....: D7 D6 D5 D4 D3 D2 B1 B0

    PORTD = (0xFC & byte);
    //PORTB = (0xFC & PORTB) | (0x03 & byte);
    PORTB = (0xFC & PORTB) | (0x03 & byte);
    //PORTB |= (0xFC |(0x03 & byte));
    strobe_WR;
    return;
}

/*uint8_t read_command(uint8_t cmd)
{
    active_CS;
    active_CMD;
    put_bus_byte(cmd);
    idle_CMD;
    uint8_t ret = get_bus_byte();
    _IDLE;
    return ret;
}*/

/*uint8_t get_bus_byte(void)
{
    _idle_DATA;
    //set data bus direction to input
    DDRB &= ~(PIN_00 | PIN_01);
    NOP;
    PORTB &= ~(PIN_00 | PIN_01);
    NOP;
    DDRD &= ~(PIN_02 | PIN_03 | PIN_04 | PIN_05 | PIN_06 | PIN_07);
    NOP;
    PORTD &= ~(PIN_02 | PIN_03 | PIN_04 | PIN_05 | PIN_06 | PIN_07);
    NOP;
    / *
 * From the data sheet: 
 * When data is written from microcomputer to GRAM, the ILI9481 writes
 * firstly to WDR, and then the data is written to GRAM automatically by
 * internal operation. Because read out operation from GRAM is conducted
 * through RDR, first read out data is invalid.
 * Normal data is read out from 2nd read out data.
 * ^^^^^^^^^^ First RD pulse is dummy!! ^^^^^^^^^^
 * /
    //latch_RD; //dummy strobe, data discarded
    {
        active_RD;
        NOP;
        idle_RD;
    }//DDRC &= ~(PIN_RD); }
    //while(!read_RD);
    //read ports
    //latch_RD;
    {
        active_RD;
        NOP;
        idle_RD;
    }
    uint8_t ret = ((0xFC & PORTD) | (0x03 & PORTB));
    //restore data bus direction to output
    DDRB |= PIN_00 | PIN_01;
    NOP;
    DDRD |= PIN_02 | PIN_03 | PIN_04 | PIN_05 | PIN_06 | PIN_07;
    NOP;
    return ret;
}*/

/******************************************************************************
 *                                                                       USART*/


/*****************************************************************************
 *                                                                     I2C LCD*/

/******************************************************************************
 *                                                                        ADC*/

/******************************************************************************
 *                                                                      TIMER0*/
ISR(TIMER0_OVF_vect)
{
    timer_counter++;
    if (timer_counter > 0x7FFF)
    {
        timer_counter = 0;
        update = true;

    }
}

/*****************************************************************************
 *                                                                      EEPROM*/

/*****************************************************************************
 *                                                                        UTIL*/

uint8_t reverse_bits(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void delay_n_us(uint16_t n)
{
    while (n--)
    {
        _delay_us(1);
    }
    return;
}

void delay_n_ms(uint16_t n)
{
    while (n--)
    {
        _delay_ms(1);
    }
    return;
}

int available_sram(void)
{
    extern int __heap_start, *__brkval;
    int v;
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
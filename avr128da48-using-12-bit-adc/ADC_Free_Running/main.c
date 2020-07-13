/*
    \file   main.c

    \brief  ADC Free-Running

    (c) 2019 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip software and any
    derivatives exclusively with Microchip products. It is your responsibility to comply with third-party
    license terms applicable to your use of third-party software (including open source software) that
    may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS
    FOR A PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS
    SOFTWARE.
*/

#define F_CPU 2000000UL     /* Main clock frequency */ 
#define START_TOKEN 0x03    /* Start Frame Token */
#define END_TOKEN 0xFC      /* End Frame Token */
/* Compute the baud rate */
#define USART1_BAUD_RATE(BAUD_RATE) (((float)F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include <stdbool.h>
#include <avr/cpufunc.h>

volatile uint16_t adcVal;

void CLKCTRL_init(void);
void PORT_init(void);
void VREF0_init(void);
void ADC0_init(void);
void USART1_init();
uint16_t ADC0_read(void);
void ADC0_start(void);
bool ADC0_conversionDone(void);
void USART1_Write(const uint8_t data);
void SYSTEM_init(void);

void CLKCTRL_init(void)
{
    /* Enable the prescaler division and set the prescaler 2 */
    ccp_write_io((void*)&(CLKCTRL.MCLKCTRLB),0x01);
}

void PORT_init(void)
{    
    /* Configure PC0 as output for USART1 TX */
    PORTC.DIRSET = PIN0_bm;
    
    /* Disable interrupt and digital input buffer on PD3 */
    PORTD.PIN3CTRL &= ~PORT_ISC_gm;
    PORTD.PIN3CTRL |= PORT_ISC_INPUT_DISABLE_gc;

    /* Disable pull-up resistor */
    PORTD.PIN3CTRL &= ~PORT_PULLUPEN_bm;
}

void VREF0_init(void)
{
    VREF.ADC0REF = VREF_REFSEL_2V048_gc;  /* Internal 2.048V reference */
}

void ADC0_init(void)
{
    ADC0.CTRLC = ADC_PRESC_DIV4_gc;        /* CLK_PER divided by 4 */
    ADC0.CTRLA = ADC_ENABLE_bm             /* ADC Enable: enabled */
               | ADC_RESSEL_12BIT_gc       /* 12-bit mode */
               | ADC_FREERUN_bm;           /* Enable Free-Run mode */
    ADC0.MUXPOS = ADC_MUXPOS_AIN3_gc;      /* Select ADC channel AIN3 <-> PD3 */
}

void USART1_init()
{
    /* Configure the baud rate: 9600 */
    USART1.BAUD = (uint16_t)USART1_BAUD_RATE(9600);
    USART1.CTRLB = USART_TXEN_bm;           /* Enable TX */
    USART1.CTRLC = USART_CHSIZE_8BIT_gc;    /* Configure character size: 8 bit */
}

uint16_t ADC0_read(void)
{
    /* Clear the interrupt flag by reading the result */
    return ADC0.RES;
}

void ADC0_start(void)
{
    /* Start ADC conversion */
    ADC0.COMMAND = ADC_STCONV_bm;
}

bool ADC0_conversionDone(void)
{
    /* Check if the conversion is done */
    return (ADC0.INTFLAGS & ADC_RESRDY_bm);
}

void USART1_Write(const uint8_t data)
{
    /* Check if USART buffer is ready to transmit data */
    while (!(USART1.STATUS & USART_DREIF_bm));
    /* Transmit data using TXDATAL register */
    USART1.TXDATAL = data;
}

void SYSTEM_init(void)
{
    CLKCTRL_init();
    PORT_init();
    VREF0_init();
    ADC0_init();
    USART1_init();
}

int main(void)
{
    SYSTEM_init();
    ADC0_start();
    
    while (1)
    {
        if (ADC0_conversionDone())
        {
            /* Read the ADC result */
            adcVal = ADC0_read();
            
            /* Transmit the ADC result to be plotted using Data Visualizer */
            USART1_Write(START_TOKEN);
            USART1_Write(adcVal & 0x00FF);
            USART1_Write(adcVal >> 8);
            USART1_Write(END_TOKEN);
        }
    }
}

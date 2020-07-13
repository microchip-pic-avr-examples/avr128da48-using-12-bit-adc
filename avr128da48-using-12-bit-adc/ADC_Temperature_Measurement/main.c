/*
    \file   main.c

    \brief  ADC Temperature Measurement

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

uint32_t temp;
int16_t temp_C;

void CLKCTRL_init(void);
void VREF0_init(void);
void ADC0_init(void);
void USART1_init(void);
int16_t ADC0_read(void);
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
}

void VREF0_init(void)
{
    VREF.ADC0REF = VREF_REFSEL_2V048_gc;  /* Internal 2.048V reference */
}

void ADC0_init(void)
{
    ADC0.CTRLC = ADC_PRESC_DIV4_gc;         /* CLK_PER divided by 4 */
    ADC0.CTRLA = ADC_ENABLE_bm              /* ADC Enable: enabled */
                | ADC_RESSEL_12BIT_gc       /* 12-bit mode */
                | ADC_FREERUN_bm;           /* Free-run mode */
    ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;  /* Select ADC channel, Temp. */
}

void USART1_init(void)
{
    /* Configure the baud rate: 9600 */
    USART1.BAUD = (uint16_t)USART1_BAUD_RATE(9600);
    USART1.CTRLB = USART_TXEN_bm;           /* Enable TX */
    USART1.CTRLC = USART_CHSIZE_8BIT_gc;    /* Configure character size: 8 bit */
}

int16_t ADC0_read(void)
{
    uint16_t sigrow_offset = SIGROW.TEMPSENSE1;
    uint16_t sigrow_slope = SIGROW.TEMPSENSE0;

    /* Clear the interrupt flag by reading the result (ADC0.RES) */
    temp = sigrow_offset - ADC0.RES;
    temp *= sigrow_slope; /* Result will overflow 16-bit variable */
    temp += 0x0800; /* Add 4096/2 to get correct rounding on division below */
    temp >>= 12; /* Round off to nearest degree in Kelvin, by dividing with 2^12 (4096) */
    return (temp - 273); /* Convert from Kelvin to Celsius (0 Kelvin - 273.15 = -273.1°C) */
}

void ADC0_start(void)
{
    /* Start conversion */
    ADC0.COMMAND = ADC_STCONV_bm;
}

bool ADC0_conversionDone(void)
{
    /* Check if the conversion is done  */
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
            temp_C = ADC0_read();
            
            /* Transmit the ADC result to be plotted using Data Visualizer */
            USART1_Write(START_TOKEN);
            USART1_Write(temp_C & 0x00FF);
            USART1_Write(temp_C >> 8);
            USART1_Write(END_TOKEN);
        }
    }
}

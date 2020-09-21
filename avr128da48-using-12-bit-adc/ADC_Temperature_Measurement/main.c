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

#define F_CPU           4000000UL   /* Main clock frequency */ 
#define START_TOKEN     0x03        /* Start Frame Token */
#define END_TOKEN       0xFC        /* End Frame Token */
/* Compute the baud rate */
#define USART1_BAUD_RATE(BAUD_RATE) (((float)F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include <stdbool.h>
#include <avr/cpufunc.h>

void CLKCTRL_init(void);
void VREF0_init(void);
void ADC0_init(void);
void USART1_init(void);
uint16_t ADC0_read(void);
int16_t temperatureConvert(uint16_t data);
void ADC0_start(void);
void USART1_Write(const uint8_t data);

/* This function initializes the CLKCTRL module */
void CLKCTRL_init(void)
{
    /* FREQSEL 4M */
    ccp_write_io((void*)&(CLKCTRL.OSCHFCTRLA), (CLKCTRL.OSCHFCTRLA | CLKCTRL_FREQSEL_4M_gc));
}

/* This function initializes the PORT module */
void PORT_init(void)
{    
    /* Configure PC0 as output for USART1 TX */
    PORTC.DIRSET = PIN0_bm;
}

/* This function initializes the VREF module */
void VREF0_init(void)
{
    VREF.ADC0REF = VREF_REFSEL_2V048_gc;  /* Internal 2.048V reference */
}

/* This function initializes the ADC module */
void ADC0_init(void)
{
    ADC0.CTRLC = ADC_PRESC_DIV4_gc;         /* CLK_PER divided by 4 */
    ADC0.CTRLA = ADC_ENABLE_bm              /* ADC Enable: enabled */
                | ADC_RESSEL_12BIT_gc       /* 12-bit mode */
                | ADC_FREERUN_bm;           /* Free-run mode */
    ADC0.MUXPOS = ADC_MUXPOS_TEMPSENSE_gc;  /* Select ADC channel, Temp. */
}

/* This function initializes the USART module */
void USART1_init(void)
{
    /* Configure the baud rate: 115200 */
    USART1.BAUD = (uint16_t)USART1_BAUD_RATE(115200);
    USART1.CTRLB = USART_TXEN_bm;           /* Enable TX */
    USART1.CTRLC = USART_CHSIZE_8BIT_gc;    /* Configure character size: 8 bit */
}

/* This function returns the ADC conversion result */
uint16_t ADC0_read(void)
{
    /* Wait for ADC result to be ready */
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
    /* Clear the interrupt flag by reading the result */
    return ADC0.RES;
}

/* This function returns the temperature value in degrees C */
int16_t temperatureConvert(uint16_t data)
{
    uint16_t sigrow_offset = SIGROW.TEMPSENSE1;
    uint16_t sigrow_slope = SIGROW.TEMPSENSE0;
    int32_t temp;
    /* Clear the interrupt flag by reading the result (ADC0.RES) */
    temp = sigrow_offset - data;
    /* Result will overflow 16-bit variable */
    temp *= sigrow_slope; 
    /* Add 4096/2 to get correct rounding on division below */
    temp += 0x0800;       
    /* Round off to nearest degree in Kelvin, by dividing with 2^12 (4096) */
    temp >>= 12;          
    /* Convert from Kelvin to Celsius (0 Kelvin - 273.15 = -273.1°C) */
    return temp - 273;      
}

/* This function starts the ADC conversions*/
void ADC0_start(void)
{
    /* Start conversion */
    ADC0.COMMAND = ADC_STCONV_bm;
}

/* This function transmits one byte through USART */
void USART1_Write(const uint8_t data)
{
    /* Check if USART buffer is ready to transmit data */
    while (!(USART1.STATUS & USART_DREIF_bm));
    /* Transmit data using TXDATAL register */
    USART1.TXDATAL = data;
}

int main(void)
{
    int16_t temp_C;
    uint16_t adcVal;
    
    /* Initialize all peripherals */
    CLKCTRL_init();
    PORT_init();
    VREF0_init();
    ADC0_init();
    USART1_init();
    
    /* Start the ADC conversion */
    ADC0_start();
    
    while (1)
    {
        /* Read the conversion result */
        adcVal = ADC0_read();
        /* Convert the ADC result in degrees C */
        temp_C = temperatureConvert(adcVal);
        
        /* Transmit the ADC result to be plotted using Data Visualizer */
        USART1_Write(START_TOKEN);
        USART1_Write(temp_C & 0x00FF);
        USART1_Write(temp_C >> 8);
        USART1_Write(END_TOKEN);
    }
}

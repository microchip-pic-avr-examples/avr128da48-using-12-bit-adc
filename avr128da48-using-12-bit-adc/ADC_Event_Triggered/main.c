/*
    \file   main.c

    \brief  ADC Event Triggered by RTC Overflow

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

/* RTC Period */
#define RTC_PERIOD      (511)       /* RTC period */
#define F_CPU           4000000UL   /* Main clock frequency */ 
#define START_TOKEN     0x03        /* Start Frame Token */
#define END_TOKEN       0xFC        /* End Frame Token */
/* Compute the baud rate */
#define USART1_BAUD_RATE(BAUD_RATE) (((float)F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/cpufunc.h>
#include <stdbool.h>

volatile uint16_t adcVal;
volatile bool adcResultReady = 0;

void CLKCTRL_init(void);
void PORT_init(void);
void VREF0_init(void);
void ADC0_init(void);
void LED0_init(void);
void USART1_init();
void LED0_toggle(void);
void USART1_Write(const uint8_t data);
void RTC_init(void);
void EVSYS_init(void);

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
    
    /* Disable interrupt and digital input buffer on PD3 */
    PORTD.PIN3CTRL &= ~PORT_ISC_gm;
    PORTD.PIN3CTRL |= PORT_ISC_INPUT_DISABLE_gc;
    
    /* Disable pull-up resistor */
    PORTD.PIN3CTRL &= ~PORT_PULLUPEN_bm;
}

/* This function initializes the VREF module */
void VREF0_init(void)
{
    VREF.ADC0REF = VREF_REFSEL_2V048_gc;  /* Internal 2.048V reference */
}

/* This function initializes the ADC module */
void ADC0_init(void)
{
    ADC0.CTRLC = ADC_PRESC_DIV4_gc;        /* CLK_PER divided by 4 */
    ADC0.CTRLA = ADC_ENABLE_bm             /* ADC Enable: enabled */
               | ADC_RESSEL_12BIT_gc;      /* 12-bit mode */
    ADC0.MUXPOS = ADC_MUXPOS_AIN3_gc;      /* Select ADC channel AIN3 <-> PD3 */
    ADC0.INTCTRL |= ADC_RESRDY_bm;         /* Enable interrupts */
    ADC0.EVCTRL |= ADC_STARTEI_bm;         /* Enable event triggered conversion */
}

/* This function initializes the LED pin */
void LED0_init(void)
{
    /* Configure the pin as output */
    PORTC.DIRSET = PIN6_bm;
    /* Configure the output high (LED == OFF) */
    PORTC.OUTSET = PIN6_bm;
}

/* This function initializes the USART module */
void USART1_init()
{
    /* Configure the baud rate: 115200 */
    USART1.BAUD = (uint16_t)USART1_BAUD_RATE(115200);
    USART1.CTRLB = USART_TXEN_bm;           /* Enable TX */
    USART1.CTRLC = USART_CHSIZE_8BIT_gc;    /* Configure character size: 8 bit */
}

/* This function toggles the LED pin */
void LED0_toggle(void)
{
    PORTC.OUTTGL = PIN6_bm;
}

/* This function transmits one byte through USART */
void USART1_Write(const uint8_t data)
{
    /* Check if USART buffer is ready to transmit data */
    while (!(USART1.STATUS & USART_DREIF_bm));
    /* Transmit data using TXDATAL register */
    USART1.TXDATAL = data;
}

ISR(ADC0_RESRDY_vect)
{
    /* Clear the interrupt flag by reading the result */
    adcVal = ADC0.RES;
	/* Update the flag */
	adcResultReady = 1;
}

/* This function initializes the RTC module  */
void RTC_init(void)
{
    /* Initialize RTC: */
    while (RTC.STATUS > 0)
    {
        ; /* Wait for all register to be synchronized */
    }

    RTC.CTRLA = RTC_PRESCALER_DIV32_gc  /* 32 */
              | RTC_RTCEN_bm            /* Enable: enabled */
              | RTC_RUNSTDBY_bm;        /* Run In Standby: enabled */
    RTC.PER = RTC_PERIOD;               /* Set period */
    RTC.CLKSEL = RTC_CLKSEL_OSC32K_gc;  /* 32.768kHz Internal Crystal (OSC32K) */
}

/* This function initializes the EVSYS module  */
void EVSYS_init(void)
{
    /* Real Time Counter overflow */
    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_RTC_OVF_gc;
    /* Connect user to event channel 0 */
    EVSYS.USERADC0START = EVSYS_USER_CHANNEL0_gc;
}

int main(void)
{
    uint16_t result;
    
    /* Initialize all peripherals */
    CLKCTRL_init();
    PORT_init();
    VREF0_init();
    ADC0_init();
    LED0_init();
    USART1_init();
    RTC_init();
    EVSYS_init();
    
    /* Enable Global Interrupts */
    sei();
    
    while (1) 
    {
        if (adcResultReady == 1)
		{
            /* Store the result to avoid altering adcVal because of the interrupt. This operation must be atomic */
            cli();
            result = adcVal;
            sei();
            
			/* Update the flag value */
			adcResultReady = 0;
			/* Toggle the LED */
			LED0_toggle();
            
			/* Transmit the ADC result to be plotted using Data Visualizer */
			USART1_Write(START_TOKEN);
			USART1_Write(result & 0x00FF);
			USART1_Write(result >> 8);
			USART1_Write(END_TOKEN);
		}

    }
}

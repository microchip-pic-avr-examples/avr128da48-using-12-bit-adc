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
#define RTC_PERIOD         (511) /* Time in ms */

#include <avr/io.h>
#include <avr/interrupt.h>

uint16_t adcVal;

void PORT_init(void);
void VREF0_init(void);
void ADC0_init(void);
void LED0_init(void);
void LED0_toggle(void);
void RTC_init(void);
void EVSYS_init(void);
void SYSTEM_init(void);

void PORT_init(void)
{
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
               | ADC_RESSEL_12BIT_gc;      /* 12-bit mode */
    ADC0.MUXPOS = ADC_MUXPOS_AIN3_gc;      /* Select ADC channel AIN3 <-> PD3 */
    ADC0.INTCTRL |= ADC_RESRDY_bm;         /* Enable interrupts */
    ADC0.EVCTRL |= ADC_STARTEI_bm;         /* Enable event triggered conversion */
}

void LED0_init(void)
{
    /* Make High (OFF) */
    PORTC.OUTSET = PIN6_bm;
    /* Make output */
    PORTC.DIRSET = PIN6_bm;
}

void LED0_toggle(void)
{
    PORTC.OUTTGL = PIN6_bm;
}

ISR(ADC0_RESRDY_vect)
{
    /* Clear the interrupt flag by reading the result */
    adcVal = ADC0.RES;
    LED0_toggle();
}

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

void EVSYS_init(void)
{
    /* Real Time Counter overflow */
    EVSYS.CHANNEL0 = EVSYS_CHANNEL0_RTC_OVF_gc;
    /* Connect user to event channel 0 */
    EVSYS.USERADC0START = EVSYS_USER_CHANNEL0_gc;
}

void SYSTEM_init(void)
{
    PORT_init();
    VREF0_init();
    ADC0_init();
    LED0_init();
    RTC_init();
    EVSYS_init();
}

int main(void)
{
    SYSTEM_init();
    
    /* Enable Global Interrupts */
    sei();
    
    while (1) 
    {
        ; /* Run in a loop */
    }
}

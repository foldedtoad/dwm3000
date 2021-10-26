/*! ----------------------------------------------------------------------------
 * @file    port.h
 * @brief   HW specific definitions and functions for portability
 *
 * @attention
 *
 * Copyright 2015 (c) DecaWave Ltd, Dublin, Ireland.
 *
 * All rights reserved.
 *
 * @author DecaWave
 */


#ifndef PORT_H_
#define PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>
#include "compiler.h"

/* DW3000 IRQ handler type. */
typedef void (*port_deca_isr_t)(void);


/*! ------------------------------------------------------------------------------------------------------------------
 * @fn port_set_dwic_isr()
 *
 * @brief This function is used to install the handling function for DW3000 IRQ.
 *
 * NOTE:
 *
 * @param deca_isr function pointer to DW1000 interrupt handler to install
 *
 * @return none
 */
void port_set_dwic_isr(port_deca_isr_t deca_isr);


/*****************************************************************************************************************//*
**/

 /****************************************************************************//**
  *
  *                                 Types definitions
  *
  *******************************************************************************/
typedef uint64_t        uint64;
typedef int64_t         int64;

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

/****************************************************************************//**
 *
 *                              MACRO
 *
 *******************************************************************************/


/****************************************************************************//**
 *
 *                              MACRO function
 *
 *******************************************************************************/

//TODO
#define GPIO_ResetBits(x,y)
#define GPIO_SetBits(x,y)
#define GPIO_ReadInputDataBit(x,y)


/* NSS pin is SW controllable */
#define port_SPIx_set_chip_select()
#define port_SPIx_clear_chip_select()

/* NSS pin is SW controllable */
#define port_SPIy_set_chip_select()
#define port_SPIy_clear_chip_select()

/****************************************************************************//**
 *
 *                              port function prototypes
 *
 *******************************************************************************/

void Sleep(uint32_t Delay);
unsigned long portGetTickCnt(void);

#define S1_SWITCH_ON  (1)
#define S1_SWITCH_OFF (0)

//when switch (S1) is 'on' the pin is low
int port_is_switch_on(uint16_t GPIOpin);
int port_is_boot1_low(void);

void port_wakeup_dw3000(void);
void port_wakeup_dw3000_fast(void);

void port_set_dw_ic_spi_slowrate(void);
void port_set_dw_ic_spi_fastrate(void);

void process_dwRSTn_irq(void);
void process_deca_irq(void);

void led_on(uint32_t led);
void led_off(uint32_t led);

int  peripherals_init(void);
void spi_peripheral_init(void);

void setup_DW3000RSTnIRQ(int enable);

void reset_DWIC(void);

extern uint32_t HAL_GetTick(void);

#ifdef __cplusplus
}
#endif

#endif /* PORT_H_ */

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
typedef uint64_t        uint64 ;

typedef int64_t         int64 ;


#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

typedef enum
{
    LED_PC6, //LED_P0.x
    LED_PC7, //LED_P0.x
    LED_PC8, //LED_P0.x
    LED_PC9, //LED_P0.x
    LED_ALL,
    LEDn
} led_t;

/****************************************************************************//**
 *
 *                              MACRO
 *
 *******************************************************************************/

//TODO
#define DW1000_RSTn                 
#define DW1000_RSTn_GPIO            


#define DECAIRQ                     
#define DECAIRQ_GPIO                

#define TA_BOOT1                    
#define TA_BOOT1_GPIO               

#define TA_RESP_DLY                 
#define TA_RESP_DLY_GPIO            

#define TA_SW1_3                      
#define TA_SW1_4                      
#define TA_SW1_5                     
#define TA_SW1_6                    
#define TA_SW1_7                   
#define TA_SW1_8                  
#define TA_SW1_GPIO               

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

void led_on(led_t led);
void led_off(led_t led);

int  peripherals_init(void);
void spi_peripheral_init(void);

void setup_DW3000RSTnIRQ(int enable);

void reset_DWIC(void);

uint32_t port_GetEXT_IRQStatus(void);
uint32_t port_CheckEXT_IRQ(void);
void port_DisableEXT_IRQ(void);
void port_EnableEXT_IRQ(void);
extern uint32_t HAL_GetTick(void);

#ifdef __cplusplus
}
#endif

#endif /* PORT_H_ */

/*
 * Taken from the Linux Kernel
 *
 */

#ifndef _LINUX_CIRC_BUF_H
#define _LINUX_CIRC_BUF_H 1

struct circ_buf {
    char *buf;
    int head;
    int tail;
};

/* Return count in buffer.  */
#define CIRC_CNT(head,tail,size) (((head) - (tail)) & ((size)-1))

/* Return space available, 0..size-1.  We always leave one free char
   as a completely full buffer has head == tail, which is the same as
   empty.  */
#define CIRC_SPACE(head,tail,size) CIRC_CNT((tail),((head)+1),(size))

/* Return count up to the end of the buffer.  Carefully avoid
   accessing head and tail more than once, so they can change
   underneath us without returning inconsistent results.  */
#define CIRC_CNT_TO_END(head,tail,size) \
    ({int end = (size) - (tail); \
      int n = ((head) + end) & ((size)-1); \
      n < end ? n : end;})

/* Return space available up to the end of the buffer.  */
#define CIRC_SPACE_TO_END(head,tail,size) \
    ({int end = (size) - 1 - (head); \
      int n = (end + (tail)) & ((size)-1); \
      n <= end ? n : end+1;})

#endif /* _LINUX_CIRC_BUF_H  */


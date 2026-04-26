#include "fun.h"
#include <stdio.h>
#include "stdint.h"
#include "stm32f1xx.h"

uint8_t key1,key1_last;
uint8_t key2,key2_last;
uint8_t key1_state = 0 , key2_state = 0;

void key_scan(void)
{
	key1=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0);
	key2=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1);
	if( key1==0 && key1_last==1 )
	{
		key1_state++;
		key1_state%=2;
	}
	if( key2==0 && key2_last==1 )
	{
		key2_state++;
		key2_state%=2;
	}
	key1_last=key1;
	key2_last=key2;
	if(key1_state)
	{
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_SET);
	}
	else
		{
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
	  }
		if(key2_state)
	{
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_SET);
	}
	else
		{
			HAL_GPIO_WritePin(GPIOA,GPIO_PIN_6,GPIO_PIN_RESET);
	  }
}
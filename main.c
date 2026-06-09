
#include "stm32mp1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "stm32mp1xx_ll_rcc.h"
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

extern int m_can_min_init_to_run(void);
//IPCC_HandleTypeDef hipcc;

//TIM_HandleTypeDef htim6;

char pPrintfBuffer[1024] ={0};
/****************************************************************************************************************************/
int printf_local( char* DebugMessage,... )
{

	    va_list ap;

	    size_t len = strlen(pPrintfBuffer);
	    size_t rem = sizeof(pPrintfBuffer) - len;

	    if (rem < 128) {//arbitrary
	        pPrintfBuffer[0] = 0;
	        len = 0;
	        rem = sizeof(pPrintfBuffer);
	    }

	    va_start(ap, DebugMessage);

	    vsnprintf(&pPrintfBuffer[len],
	              rem,
	              DebugMessage,
	              ap);

	    va_end(ap);

	    return 0;
}
/****************************************************************************************************************************/
static void Endlessloop(void) {
    int volatile debugger_attached = 0;

    while (0==debugger_attached) {
        __NOP();
    }
}
/****************************************************************************************************************************/
int main(void)
{
	Endlessloop();
    m_can_min_init_to_run();

    while (1)
        __NOP();

    return 0;
}

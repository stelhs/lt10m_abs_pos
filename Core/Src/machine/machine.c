#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "main.h"
#include "stm32_lib/crc32.h"
#include "machine.h"
#include "tim.h"
#include "usart.h"
#include "spi.h"

static int is_requested = FALSE;
static volatile u32 freq1, freq2 = 0;

int _write(int file, char *str, int len)
{
    size_t i=0;
    for(i = 0; i < len; i++)
        HAL_UART_Transmit(&huart1, (u8 *)str + i, 1, 0xFFFF);
    return len;
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_3) { // NSS pin (joined with PA4)
        is_requested = TRUE;
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    u32 val1, val2;
    if (htim == &htim1) {
        val1 = htim3.Instance->CNT;
        val2 = htim4.Instance->CNT;
        htim3.Instance->CNT = 0;
        htim4.Instance->CNT = 0;
        freq1 = val1;
        freq2 = val2;
    }
}

void machine_init(void)
{
    u32 tx[5];
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
    __HAL_TIM_CLEAR_FLAG(&htim5, TIM_FLAG_UPDATE);

    HAL_TIM_Base_Start_IT(&htim1);
    HAL_TIM_Base_Start_IT(&htim2);
    HAL_TIM_Base_Start_IT(&htim5);
    HAL_TIM_Base_Start_IT(&htim3);
    HAL_TIM_Base_Start_IT(&htim4);

    printf("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
    printf("Start abs_position ver: %s\r\n", BUILD_VERSION);
    for(;;) {
    //    printf("%d %d\r\n", freq1, freq2);
        if(!is_requested)
            continue;
        is_requested = FALSE;
        tx[0] = __HAL_TIM_GET_COUNTER(&htim5);
        tx[1] = __HAL_TIM_GET_COUNTER(&htim2);
        tx[2] = freq1 * 4;
        tx[3] = freq2 * 4;
        tx[4] = crc32(tx, 4 * 4);
        HAL_SPI_Transmit(&hspi1, (u8*)tx, sizeof tx, 10);
    }
}

#include <stdio.h>
#include "stm32f4xx_hal.h"
#include "main.h"
#include "stm32_lib/crc32.h"
#include "machine.h"
#include "tim.h"
#include "usart.h"
#include "spi.h"

static int is_requested = FALSE;

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

void machine_init(void)
{
    u8 tx[12];
    u32 *pos1 = (u32 *)tx;
    u32 *pos2 = (u32 *)(tx + 4);
    u32 *crc = (u32 *)(tx + 8);
    HAL_TIM_Base_Start_IT(&htim2);
    HAL_TIM_Base_Start_IT(&htim5);

    printf("\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n");
    printf("Start abs_position ver: %s\r\n", BUILD_VERSION);
    for(;;) {
        if(!is_requested)
            continue;
        is_requested = FALSE;
        *pos1 = __HAL_TIM_GET_COUNTER(&htim5);
        *pos2 = __HAL_TIM_GET_COUNTER(&htim2);
        *crc = crc32(tx, 8);
        HAL_SPI_Transmit(&hspi1, tx, 12, 10);
    }
}

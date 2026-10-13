#include "smartwatch.h"
#include "stm32u5x9j_discovery.h"
#include "led_button_demo.h"
#include "device_runtime.h"
#include "device_contract.h"
#include "services/watch_display_service.h"
#include "watch_ui.h"
#include "scene.h"
volatile uint32_t smartwatch_status,smartwatch_heartbeat,smartwatch_rtc_lse;
__attribute__((section(".app_info"),used))
const device_app_info_t device_app_info={DEVICE_APP_MAGIC,DEVICE_APP_DUELLO,1,DEVICE_PROTOCOL};
static void SystemClock_Config(void);
/* This application is controlled by the PC; no watch navigation is linked. */
void watch_ui_touch(lv_indev_data_t *data,uint32_t now){(void)data;(void)now;}
int main(void){
    smartwatch_status=1;HAL_Init();SystemClock_Config();
    if(HAL_ICACHE_WaitForInvalidateComplete()!=HAL_OK || HAL_ICACHE_Enable()!=HAL_OK)Error_Handler();
    BSP_LED_Init(LED_RED);watch_platform_display_init();duello_scene_init();
    led_button_demo_init();device_runtime_init(&led_demo_uart,"Duello",duello_command);smartwatch_status=4;
    while(1){led_button_demo_poll();device_runtime_poll();watch_platform_display_poll();lv_timer_handler();++smartwatch_heartbeat;HAL_Delay(1);}
}
void Error_Handler(void){smartwatch_status|=0x80000000;while(1){BSP_LED_Toggle(LED_RED);HAL_Delay(200);}}
void assert_failed(uint8_t *f,uint32_t l){(void)f;(void)l;Error_Handler();}
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* Enable voltage range 1 for frequency above 100 Mhz */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Switch to SMPS regulator instead of LDO */
  HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY);

  __HAL_RCC_PWR_CLK_DISABLE();

  /* MSI Oscillator enabled at reset (4Mhz), activate PLL with MSI as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI | RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_4;
  RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 80;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLFRACN= 0;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Select PLL as system clock source and configure bus clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | \
                                 RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_PCLK3);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f4xx.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_PIN   13u
#define BTN_PIN   0u
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static volatile uint32_t msTicks        = 0;
static volatile uint32_t lastToggle     = 0;
static volatile uint32_t lastButtonEdge = 0;

// Set of flashing periods (ms); the button cycles through them
static const uint32_t periods[] = { 100, 250, 500, 1000, 2000 };
static const uint8_t  periodsCount = sizeof(periods) / sizeof(periods[0]);
static volatile uint8_t periodIndex = 2; /* start — 500 ms */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// SysTick: 1 ms system timer, used as a "clock"
void SysTick_Handler(void)
{
    msTicks++;
}

// GPIO init
static void GPIO_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOAEN;

    // PC13 — output push-pull, low speed enough for the LED
    GPIOC->MODER   &= ~(0x3u << (LED_PIN * 2));
    GPIOC->MODER   |=  (0x1u << (LED_PIN * 2));   // 01 = General purpose output
    GPIOC->OTYPER  &= ~(0x1u << LED_PIN);         //  0  = push-pull
    GPIOC->OSPEEDR &= ~(0x3u << (LED_PIN * 2));   // low speed
    GPIOC->ODR     |=  (0x1u << LED_PIN);         // LED active-LOW -> off

    // PA0 — input with pull-up to VCC (button pulls the line to GND)
    GPIOA->MODER &= ~(0x3u << (BTN_PIN * 2)); // 00 = input
    GPIOA->PUPDR &= ~(0x3u << (BTN_PIN * 2));
    GPIOA->PUPDR |= (0x1u << (BTN_PIN * 2)); // 01 = pull-up
}

// EXTI0 init on PA0
static void EXTI_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // EXTI0 connect to port A (PA0)
    SYSCFG->EXTICR[0] &= ~(0xFu << 0);
    SYSCFG->EXTICR[0] |=  (0x0u << 0); // 0000 = PA0

    EXTI->IMR  |= (1u << BTN_PIN);   // allow interrupt on line 0
    EXTI->FTSR |= (1u << BTN_PIN);   // reaction to falling edge (press)
    EXTI->RTSR &= ~(1u << BTN_PIN);

    NVIC_SetPriority(EXTI0_IRQn, 1);
    NVIC_EnableIRQ(EXTI0_IRQn);
}

// Button interrupt handler
void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & (1u << BTN_PIN))
    {
        EXTI->PR = (1u << BTN_PIN); // clear flag

        // software debounce — ignore repeated activations during 200 ms
        if ((msTicks - lastButtonEdge) > 200u)
        {
            lastButtonEdge = msTicks;
            periodIndex = (uint8_t)((periodIndex + 1u) % periodsCount);
        }
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000u); // SysTick tick every 1 ms

    GPIO_Init();
    EXTI_Init();

    while (1)
    {
        uint32_t currentPeriod = periods[periodIndex];

        if ((msTicks - lastToggle) >= currentPeriod)
        {
            lastToggle = msTicks;
            GPIOC->ODR ^= (1u << LED_PIN); // toggle LED
        }
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

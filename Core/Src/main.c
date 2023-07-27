/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "Common.h"
#include "Leds.h"
#include "Effect.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LED_AMOUNT 6
#define GPIO_PIN_LEFT GPIO_PIN_13
#define GPIO_PIN_INDEX_LEFT 0
#define BUTTON_PRESSED 0
#define BUTTON_NOT_PRESSED 1
#define IWDG_TIMEOUT 1638 // ms
#define RESET_HOLD_TIME (uint8_t)(IWDG_TIMEOUT / 16.65)
#define MINIMAL_HOLD_TIME (uint8_t)(400 / 16.65)
#define MINIMAL_CLICK_TIME (uint8_t)(40 / 16.65)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
IWDG_HandleTypeDef hiwdg;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint16_t gLedBuffer[24 * (LED_AMOUNT + 4)];
uint8_t gPinState[2] = {0, 0};
uint16_t gPinHoldTime[2] = {0, 0};
static const COLOR_GRB Miku = {212, 1, 59};
static const COLOR_GRB Kaito = {9, 34, 255};
static const COLOR_GRB Yellow = {229, 216, 0};
static const COLOR_GRB Orange = {150, 255, 0};
static const COLOR_GRB Red = {4, 189, 4};
static const COLOR_GRB Luka = {105, 255, 180};
static const COLOR_GRB Gumi = {255, 25, 25};
static const COLOR_GRB White = {255, 255, 255};
static const COLOR_GRB Ai = {0, 75, 130};
static PALLETE gMikuSolidPallete[] = {{0, Miku}};
static PALLETE gKaitoSolidPallete[] = {{0, Kaito}};
static PALLETE gYellowSolidPallete[] = {{0, Yellow}};
static PALLETE gOrangeSolidPallete[] = {{0, Orange}};
static PALLETE gRedSolidPallete[] = {{0, Red}};
static PALLETE gLukaSolidPallete[] = {{0, Luka}};
static PALLETE gGumiSolidPallete[] = {{0, Gumi}};
static PALLETE gWhiteSolidPallete[] = {{0, White}};
static PALLETE gAiSolidPallete[] = {{0, Ai}};
static PALLETE gRedTransitivePallete[] = {{0, Red}, {50, Miku}, {170, Kaito}, {255, Red}};
static PALLETE gGreenTransitivePallete[] = {{0, Ai}, {128, White}, {255, Ai}};
static PALLETE gBlueTransitivePallete[] = {{0, Miku}, {60, Miku}, {128, Luka}, {180, Luka}, {255, Miku}};
static PALLETE_ARRAY gTransitivePalletes[] = {
  {gGreenTransitivePallete, LENGTH_OF (gGreenTransitivePallete)},
  {gBlueTransitivePallete, LENGTH_OF (gBlueTransitivePallete)},
  {gRedTransitivePallete, LENGTH_OF (gRedTransitivePallete)}};
static PALLETE_ARRAY gSolidPalletes[] = {
                                        {gMikuSolidPallete, 1},
                                        {gKaitoSolidPallete, 1},
                                        {gYellowSolidPallete , 1},
                                        {gOrangeSolidPallete, 1},
                                        {gRedSolidPallete, 1},
                                        {gLukaSolidPallete, 1},
                                        {gGumiSolidPallete, 1},
                                        {gWhiteSolidPallete, 1},
                                        {gAiSolidPallete, 1}
                                        };
static const uint8_t gLedBufferSize = LENGTH_OF (gLedBuffer);
static uint8_t gEffectsIndex = 0;
static uint8_t gPalleteIndex = 0;
static uint8_t TurnOff = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_IWDG_Init(void);
/* USER CODE BEGIN PFP */
static void StartLedsDma(void);
static void SendUartBlockingMessage(const char*);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void ShowEffectRainbowWrapper(void) {
  ShowEffectRainbow(0, 6, 2);
}

static void ShowEffectPalleteSmoothTransitionWrapper(void) {
  COLOR_GRB Color;
  char message[40];
  Color = ShowEffectPalleteSmoothTransition(0, 1, &gTransitivePalletes[gPalleteIndex % LENGTH_OF (gTransitivePalletes)]);
  sprintf(message, "RGB: %d, %d, %d\r\n", Color.Red, Color.Green, Color.Blue);
  SendUartBlockingMessage (message);
}

static void ShowEffectPalleteInstantTransitionWrapper(void) {
  ShowEffectPalleteInstantTransition(0, 2, &gSolidPalletes[gPalleteIndex % LENGTH_OF (gSolidPalletes)]);
}

static void ShowEffectBrightnessWrapper(void) {
  ShowEffectBrightness(0, ((gPalleteIndex * 10) % 50) + MINIMAL_BRIGHTNESS);
}

static void (*gEffects[])(void) = {ShowEffectPalleteSmoothTransitionWrapper, ShowEffectRainbowWrapper, ShowEffectBrightnessWrapper};
static const uint8_t gEffectsSize = LENGTH_OF (gEffects);

static void UpdatePalleteIndex(uint8_t Increase) {
  if (Increase == 0) {
    gPalleteIndex++;
  } else {
    gPalleteIndex--;
  }
}

static void UpdateEffectsIndex(uint8_t Increase) {
  if (Increase == 0) {
    gEffectsIndex++;
  } else {
    gEffectsIndex--;
  }
}

static void UpdatePinLogic(uint16_t GpioPin, uint8_t GpioIndex) {
  uint8_t GpioPinCurrentState = HAL_GPIO_ReadPin(GPIOC, GpioPin);
  if (GpioPinCurrentState != gPinState[GpioIndex]) {
    if (gPinState[GpioIndex] == BUTTON_PRESSED) {
      if (gPinHoldTime[GpioIndex] < RESET_HOLD_TIME && gPinHoldTime[GpioIndex] >= MINIMAL_HOLD_TIME) {
        UpdateEffectsIndex(GpioIndex);
      } else if (gPinHoldTime[GpioIndex] < MINIMAL_HOLD_TIME && gPinHoldTime[GpioIndex] >= MINIMAL_CLICK_TIME) {
        UpdatePalleteIndex(GpioIndex);
      }
    } else if (gPinState[GpioIndex] == BUTTON_NOT_PRESSED) {
      if (gPinHoldTime[GpioIndex] >= RESET_HOLD_TIME) {
        // HAL_PWR_EnterSTANDBYMode();
      }
      gPinHoldTime[GpioIndex] = 0;
    }
  } else if (GpioPinCurrentState == BUTTON_PRESSED) {
    if (gPinHoldTime[GpioIndex] >= RESET_HOLD_TIME) {
      // SendUartBlockingMessage("T\r\n");
      TurnOff = 1;
    }
    gPinHoldTime[GpioIndex]++;
  }

  gPinState[GpioIndex] = GpioPinCurrentState;
}

static void UpdateLeds() {
  if (hdma_tim1_ch1.State == HAL_DMA_STATE_READY) {
    if (TurnOff) {
      TurnOffLeds(0);
    } else {
      gEffects[gEffectsIndex % gEffectsSize]();
    }
    PrepareBufferForTransaction(0);
    // char message[40];
    // sprintf();
    // SendUartBlockingMessage("%d\r\n", gEffectsIndex);
    StartLedsDma();
  }
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
  if (htim == &htim1) {
    HAL_TIM_PWM_Stop_DMA (&htim1, TIM_CHANNEL_1);
  }
}

// single click - change color
// single hold - change effect
// single long (1.6s) hold - turn off
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  if (htim == &htim2) {
    HAL_IWDG_Refresh(&hiwdg);
    UpdatePinLogic(GPIO_PIN_LEFT, GPIO_PIN_INDEX_LEFT);
    UpdateLeds();
  }
}

static void StartLedsDma(void) {
  HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t*)gLedBuffer, gLedBufferSize);
}

static void SendUartBlockingMessage(const char* message) {
  HAL_UART_Transmit(&huart2, (uint8_t*)message, (uint16_t)strlen(message), 100);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  MX_GPIO_Init();
  // if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_LEFT) == BUTTON_NOT_PRESSED) {
  //   MX_IWDG_Init();
    // HAL_PWR_EnterSTANDBYMode();
  // }
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
  srand(69);
  InitializeConfigs(1);
  InitializeConfig(0, LED_AMOUNT, NULL, gLedBuffer, gLedBufferSize);
  for (uint8_t Index = 0; Index < LED_AMOUNT; Index++) {
    // GetLedSection(0, Index)->Color = (COLOR_GRB){206,134,203};
    GetLedSection(0, Index)->Color = (COLOR_GRB){255,255,20};
  }
  PrepareBufferForTransaction(0);
  SendUartBlockingMessage("STM initialized\r\n");

  char message[100];
  COLOR_HSV Green = RgbToHsv((COLOR_GRB){191, 191, 191});
  COLOR_HSV Red = RgbToHsv((COLOR_GRB){0, 127, 0});
  sprintf(message, "Rgb to hsv Green: %d %d %d Red: %d %d %d\r\n", Green.h, Green.s, Green.v, Red.h, Red.s, Red.v);
  SendUartBlockingMessage(message);
  StartLedsDma();
  HAL_Delay(1000);
  SendUartBlockingMessage("STM started\r\n");
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_PWR_EnableSleepOnExit();
  HAL_PWR_EnterSLEEPMode(0, PWR_SLEEPENTRY_WFI);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief IWDG Initialization Function
  * @param None
  * @retval None
  */
static void MX_IWDG_Init(void)
{

  /* USER CODE BEGIN IWDG_Init 0 */

  /* USER CODE END IWDG_Init 0 */

  /* USER CODE BEGIN IWDG_Init 1 */

  /* USER CODE END IWDG_Init 1 */
  hiwdg.Instance = IWDG;
  hiwdg.Init.Prescaler = IWDG_PRESCALER_16;
  hiwdg.Init.Reload = 4095;
  if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN IWDG_Init 2 */

  /* USER CODE END IWDG_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 79;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 19;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 53332;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

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

#ifdef  USE_FULL_ASSERT
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

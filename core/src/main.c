/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include <stdlib.h> // Do funkcji rand() i srand()
#include <stdbool.h> // Do typu bool
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SEG_A_Pin GPIO_PIN_0
#define SEG_A_GPIO_Port GPIOD
#define SEG_B_Pin GPIO_PIN_1
#define SEG_B_GPIO_Port GPIOD
#define SEG_C_Pin GPIO_PIN_2
#define SEG_C_GPIO_Port GPIOD
#define SEG_D_Pin GPIO_PIN_3
#define SEG_D_GPIO_Port GPIOD
#define SEG_E_Pin GPIO_PIN_8
#define SEG_E_GPIO_Port GPIOD
#define SEG_F_Pin GPIO_PIN_9
#define SEG_F_GPIO_Port GPIOD
#define SEG_G_Pin GPIO_PIN_10
#define SEG_G_GPIO_Port GPIOD
#define SEG_DP_Pin GPIO_PIN_11
#define SEG_DP_GPIO_Port GPIOD
#define LOW GPIO_PIN_RESET
#define HIGH GPIO_PIN_SET
// Pin dla buzzera (PWM na TIM1 Channel 1)
#define BUZZER_PWM_CHANNEL TIM_CHANNEL_1
#define BUZZER_PWM_TIM htim1

// Czas trwania dźwięku buzzera (w ms)
#define BUZZER_DURATION_MS 50

// Określa, czy przycisk został naciśnięty
volatile bool buttonPressed = false;

#define ANIMATION_DURATION_MS 1000  // Czas trwania animacji losowania (1 sekunda)
#define ANIMATION_STEP_DELAY_MS 50   // Opóźnienie między zmianą cyfr w animacji (50 ms)
#define BUZZER_TICK_DURATION_MS 10   // Czas trwania pojedynczego "kliku" buzzera

// Dodatkowe definicje dla diod LED (już istniejące w sekcji GPIO_Init, ale powtarzamy dla jasności)
#define LD4_Pin GPIO_PIN_12 // Zielona
#define LD3_Pin GPIO_PIN_13 // Pomarańczowa
#define LD5_Pin GPIO_PIN_14 // Czerwona
#define LD6_Pin GPIO_PIN_15 // Niebieska
#define LED_GPIO_Port GPIOD // Wszystkie diody są na porcie D

// Dodana zmienna do zarządzania stanem animacji
volatile bool animationRunning = false;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */
void displayDigit(int digit);
void setSegment(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState state);

// Funkcje do sterowania buzzerem
void buzzer_on(void);
void buzzer_off(void);

// Funkcja obsługi przerwania przycisku
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
const uint8_t segmentPatterns[10][8] = {
    //   A    B    C    D    E    F    G    DP (High = OFF)
    {LOW, LOW, LOW, LOW, LOW, LOW, HIGH, HIGH}, // 0
    {HIGH, LOW, LOW, HIGH, HIGH, HIGH, HIGH, HIGH}, // 1  <-- WZORZEC DLA CYFRY 1
    {LOW, LOW, HIGH, LOW, LOW, HIGH, LOW, HIGH}, // 2
    {LOW, LOW, LOW, LOW, HIGH, HIGH, LOW, HIGH}, // 3
    {HIGH, LOW, LOW, HIGH, HIGH, LOW, LOW, HIGH}, // 4
    {LOW, HIGH, LOW, LOW, HIGH, LOW, LOW, HIGH}, // 5
    {LOW, HIGH, LOW, LOW, LOW, LOW, LOW, HIGH}, // 6
    {LOW, LOW, LOW, HIGH, HIGH, HIGH, HIGH, HIGH}, // 7
    {LOW, LOW, LOW, LOW, LOW, LOW, LOW, HIGH}, // 8
    {LOW, LOW, LOW, LOW, HIGH, LOW, LOW, HIGH}  // 9
};

/**
 * @brief Ustawia stan wyjścia GPIO dla danego segmentu.
 * @param GPIOx: Port GPIO (np. GPIOD)
 * @param GPIO_Pin: Pin GPIO (np. GPIO_PIN_0)
 * @param state: Stan pinu (GPIO_PIN_SET dla OFF, GPIO_PIN_RESET dla ON w Common Anode)
 */
void setSegment(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState state) {
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, state);
}

/**
 * @brief Wyświetla cyfrę na 8-segmentowym wyświetlaczu (Common Anode).
 * @param digit: Cyfra do wyświetlenia (0-9).
 */
void displayDigit(int digit) {
    if (digit < 0 || digit > 9) {
        // Wszystkie segmenty OFF (HIGH)
        setSegment(SEG_A_GPIO_Port, SEG_A_Pin, HIGH);
        setSegment(SEG_B_GPIO_Port, SEG_B_Pin, HIGH);
        setSegment(SEG_C_GPIO_Port, SEG_C_Pin, HIGH);
        setSegment(SEG_D_GPIO_Port, SEG_D_Pin, HIGH);
        setSegment(SEG_E_GPIO_Port, SEG_E_Pin, HIGH);
        setSegment(SEG_F_GPIO_Port, SEG_F_Pin, HIGH);
        setSegment(SEG_G_GPIO_Port, SEG_G_Pin, HIGH);
        setSegment(SEG_DP_GPIO_Port, SEG_DP_Pin, HIGH);
        return;
    }

    // Ustawia piny segmentów na podstawie tablicy segmentPatterns
    HAL_GPIO_WritePin(SEG_A_GPIO_Port, SEG_A_Pin, segmentPatterns[digit][0]); // PD0 (Segment A)
    HAL_GPIO_WritePin(SEG_B_GPIO_Port, SEG_B_Pin, segmentPatterns[digit][1]); // PD1 (Segment B)
    HAL_GPIO_WritePin(SEG_C_GPIO_Port, SEG_C_Pin, segmentPatterns[digit][2]); // PD2 (Segment C)
    HAL_GPIO_WritePin(SEG_D_GPIO_Port, SEG_D_Pin, segmentPatterns[digit][3]); // PD3 (Segment D)
    HAL_GPIO_WritePin(SEG_E_GPIO_Port, SEG_E_Pin, segmentPatterns[digit][4]); // PD8 (Segment E)
    HAL_GPIO_WritePin(SEG_F_GPIO_Port, SEG_F_Pin, segmentPatterns[digit][5]); // PD9 (Segment F)
    HAL_GPIO_WritePin(SEG_G_GPIO_Port, SEG_G_Pin, segmentPatterns[digit][6]); // PD10 (Segment G)
    HAL_GPIO_WritePin(SEG_DP_GPIO_Port, SEG_DP_Pin, segmentPatterns[digit][7]);// PD11 (Segment DP)
}
/**
 * @brief Włącza buzzer na określoną częstotliwość i czas.
 * Ustawia PWM Pulse na 50%, aby wygenerować dźwięk.
 * @param freq_hz: Częstotliwość dźwięku w Hz (0 = wyłącz).
 * @param duration_ms: Czas trwania dźwięku w milisekundach.
 */
void buzzer_play_freq(uint16_t freq_hz, uint32_t duration_ms) {
    if (freq_hz == 0) { // Jeśli częstotliwość to 0, wyłącz buzzer
        buzzer_off();
        return;
    }

    // Pobierz częstotliwość zegara TIM1 (jest na APB2, 100MHz HCLK -> 100MHz PCLK2)
    // UWAGA: Jeśli Twoj zegar działa na HSI (16MHz), PCLK2 to też 16MHz.
    // Wtedy te obliczenia będą inne i dźwięk może być inny.
    // Standardowo PCLK2 = HAL_RCC_GetPCLK2Freq(); ale to dynamiczne.
    // Dla stałego 100MHz HCLK i DIV1, PCLK2_Timer_Clock = 100 000 000 Hz.
    // Dla Twojego obecnego kodu (HSI 16MHz, PCLK2 16MHz), PCLK2_Timer_Clock = 16 000 000 Hz.
    uint32_t timer_base_clock = HAL_RCC_GetPCLK2Freq();
        uint16_t new_period = (timer_base_clock / freq_hz) - 1;

    // Ustaw nowy Period (ARR) dla TIM1
    __HAL_TIM_SET_AUTORELOAD(&BUZZER_PWM_TIM, new_period);
    // Ustaw wypełnienie PWM na 50%
    __HAL_TIM_SET_COMPARE(&BUZZER_PWM_TIM, BUZZER_PWM_CHANNEL, new_period / 2);

    HAL_TIM_PWM_Start(&BUZZER_PWM_TIM, BUZZER_PWM_CHANNEL);

    if (duration_ms > 0) {
        HAL_Delay(duration_ms);
        buzzer_off();
    }
}
/**
 * @brief Włącza buzzer.
 * Ustawia PWM Pulse na 50%, aby wygenerować dźwięk.
 */
void buzzer_on(void) {
    // Ustawienie Pulse na połowę Period (50% wypełnienia)
    __HAL_TIM_SET_COMPARE(&BUZZER_PWM_TIM, BUZZER_PWM_CHANNEL, BUZZER_PWM_TIM.Init.Period / 2);
    HAL_TIM_PWM_Start(&BUZZER_PWM_TIM, BUZZER_PWM_CHANNEL);
}

/**
 * @brief Wyłącza buzzer.
 * Ustawia PWM Pulse na 0.
 */
void buzzer_off(void) {
    __HAL_TIM_SET_COMPARE(&BUZZER_PWM_TIM, BUZZER_PWM_CHANNEL, 0);
    HAL_TIM_PWM_Stop(&BUZZER_PWM_TIM, BUZZER_PWM_CHANNEL);
}

/**
 * @brief Obsługa przerwania zewnętrznego dla przycisku (PA0).
 * Ustawia flagę buttonPressed na true.
 * @param GPIO_Pin: Pin, który wywołał przerwanie.
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_0) { // Sprawdź, czy przerwanie pochodzi z PA0 (USER_BUTTON)
        // Proste debouncing (można rozbudować o timer)
        static uint32_t last_press_time = 0;
        uint32_t current_time = HAL_GetTick();

        if (current_time - last_press_time > 200) { // Odczekaj 200ms na debouncing
            buttonPressed = true;
            last_press_time = current_time;
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

  /* USER CODE BEGIN 1 */
	int randomNumber = 1;

	  // Struktura do odczytu czasu z RTC
	  RTC_TimeTypeDef sTime = {0};
	  RTC_DateTypeDef sDate = {0}; // Data jest wymagana do odczytu czasu, nawet jeśli jej nie używamy
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_RTC_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }
    // Data również musi być odczytana, aby czas był aktualizowany w tle
    if (HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
        Error_Handler();
    }
    // Użyj bieżących sekund z RTC jako ziarna dla funkcji rand()
    srand(sTime.Seconds + sTime.Minutes * 60 + sTime.Hours * 3600 + HAL_GetTick()); // Dodano HAL_GetTick() dla lepszej "losowości"

    // ************* NA START: WYŚWIETL CYFRĘ 1 *************
  /*  displayDigit(1); // To jest funkcja, która wyświetli cyfrę 1
    HAL_Delay(1000);
    displayDigit(2); // To jest funkcja, która wyświetli cyfrę 1
        HAL_Delay(1000);
        displayDigit(3); // To jest funkcja, która wyświetli cyfrę 1
            HAL_Delay(1000);
            displayDigit(4); // To jest funkcja, która wyświetli cyfrę 1
                HAL_Delay(1000);
                displayDigit(5); // To jest funkcja, która wyświetli cyfrę 1
                    HAL_Delay(1000);
                    displayDigit(6); // To jest funkcja, która wyświetli cyfrę 1
                        HAL_Delay(1000);
                        displayDigit(7); // To jest funkcja, która wyświetli cyfrę 1
                            HAL_Delay(1000);
                            displayDigit(8); // To jest funkcja, która wyświetli cyfrę 1
                                HAL_Delay(1000);
                                displayDigit(9); // To jest funkcja, która wyświetli cyfrę 1
                                    HAL_Delay(1000);
                                    displayDigit(0); // To jest funkcja, która wyświetli cyfrę 1
                                        HAL_Delay(1000);
                                        */
    // *******************************************************
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if (buttonPressed) {
	        // Wyzeruj flagę, aby uniknąć wielokrotnego wykonania
	        buttonPressed = false;

	        // 1. Dźwięk początkowy (krótki "klik")
	        buzzer_play_freq(1000, BUZZER_DURATION_MS); // Gra 1kHz przez 50ms

	        // 2. Rozpoczęcie animacji losowania
	        animationRunning = true;
	        uint32_t animationStartTime = HAL_GetTick(); // Zapisz czas rozpoczęcia animacji
	        uint32_t nextAnimationStepTime = animationStartTime;

	        // Zmienne dla narastającego tonu buzzera
	        uint16_t current_freq = 300; // Częstotliwość startowa
	        uint16_t freq_step = 50;     // Krok zwiększania częstotliwości

	        // DODANE: Piny LEDów Discovery do animacji (PD12, PD13, PD14, PD15)
	        // Array do łatwego przełączania LEDów
	        uint16_t allLeds[] = {LD4_Pin, LD3_Pin, LD5_Pin, LD6_Pin}; // Kolejność: Zielony, Pomarańczowy, Czerwony, Niebieski
	        int currentLedIndex = 0; // Indeks aktualnie świecącej diody

	        while (HAL_GetTick() - animationStartTime < ANIMATION_DURATION_MS) {
	            if (HAL_GetTick() >= nextAnimationStepTime) {
	                nextAnimationStepTime += ANIMATION_STEP_DELAY_MS;

	                // Losuj i wyświetl losową cyfrę (1-6) na potrzeby animacji
	                uint8_t current_animation_digit = (rand() % 6) + 1;
	                displayDigit(current_animation_digit);

	                // Buzzer: narastający ton
	                buzzer_play_freq(current_freq, BUZZER_TICK_DURATION_MS); // Krótki dźwięk
	                current_freq += freq_step; // Zwiększ częstotliwość
	                if (current_freq > 2000) { // Zapobiegnij zbyt wysokim częstotliwościom
	                    current_freq = 300; // Resetuj częstotliwość
	                }

	                // DODANE: Diody LED: zaświeć jedną w kółko
	                // Wyłącz wszystkie diody najpierw, potem włącz jedną
	                HAL_GPIO_WritePin(LED_GPIO_Port, LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin, GPIO_PIN_SET); // Wszystkie OFF (HIGH)
	                HAL_GPIO_WritePin(LED_GPIO_Port, allLeds[currentLedIndex], GPIO_PIN_RESET); // Włącz aktualną LED (LOW = ON)

	                currentLedIndex = (currentLedIndex + 1) % 4; // Przejdź do następnej LED w cyklu (0, 1, 2, 3, 0...)
	            }
	        }
	        animationRunning = false; // Zakończ animację

	        // Wyłącz buzzer po animacji
	        buzzer_off();

	        // DODANE: Wyłącz wszystkie diody LED po zakończeniu animacji
	        HAL_GPIO_WritePin(LED_GPIO_Port, LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin, GPIO_PIN_SET); // Wszystkie OFF (HIGH)

	        // 3. Po zakończeniu animacji, wylosuj ostateczną liczbę i wyświetl ją
	        randomNumber = (rand() % 6) + 1;
	        displayDigit(randomNumber);

	        // 4. Dźwięk końcowy (np. dłuższy, wyższy ton)
	        buzzer_play_freq(1500, 150); // Gra 1.5kHz przez 150ms
	        buzzer_off(); // Upewnij się, że jest wyłączony
	        HAL_Delay(200); // Krótka pauza po zakończeniu rzutu
	  }
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
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

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
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD9 PD10 PD11
                           PD12 PD13 PD14 PD15
                           PD0 PD1 PD2 PD3 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11
                          |GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

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

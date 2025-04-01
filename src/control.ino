


// Временные параметры
const unsigned long debounceDelay = 50;    // Время антидребезга (мс)
const unsigned long longPressDelay = 2000; // Время длинного нажатия (мс)
const unsigned long veryLongPressDelay = 5000; // Очень длинное нажатие
const unsigned long resetPressDelay = 10000;   // Нажатие для сброса

// Флаги состояния
bool pushup_pushed = false;    // Флаг нажатия кнопки вверх
bool pushdown_pushed = false;  // Флаг нажатия кнопки вниз
bool main_pushed = false;      // Флаг нажатия основной кнопки
bool long_press = false;       // Флаг длинного нажатия

// =============================================
// ОСНОВНАЯ ФУНКЦИЯ УПРАВЛЕНИЯ
// =============================================

void control() {
  // Отдельные таймеры для каждой кнопки
  static unsigned long main_button_timer = 0;
  static unsigned long pushup_timer = 0;
  static unsigned long pushdown_timer = 0;
  
  processSerialCommands(); // Обработка команд из последовательного порта
  
  // Текущее время (для оптимизации вызовов millis())
  unsigned long current_time = millis();

  // ===========================================
  // 1. ОБРАБОТКА ОСНОВНОЙ КНОПКИ (PUSH_PIN)
  // ===========================================
  
  // Чтение состояния кнопки с антидребезгом
  static bool main_button_state = false;
  static bool main_button_last = false;
  static unsigned long main_button_debounce = 0;
  
  bool main_button_read = digitalRead(push_pin) == HIGH; // Предполагаем HIGH - нажата
  
  // Обнаружение изменения состояния
  if (main_button_read != main_button_last) {
    main_button_debounce = current_time;
  }
  
  // Если состояние стабильно дольше debounceDelay
  if (current_time - main_button_debounce >= debounceDelay) {
    if (main_button_read != main_button_state) {
      main_button_state = main_button_read;
      
      // Обработка нажатия (передний фронт)
      if (main_button_state && !main_pushed) {
        main_pushed = true;
        main_button_timer = current_time;
        long_press = false;
      }
      
      // Обработка отпускания (задний фронт).  Включение и выключение нагревателя.
      if (!main_button_state && main_pushed) {
        // Если было короткое нажатие
        if (!long_press) {
          if (burn_mode == 1 || burn_mode == 2) {
            burn = false;  // Выключаем нагреватель
          } else {
            burn = true;   // Включаем нагреватель
          }
        }
        main_pushed = false;
        long_press = false;
      }
    }
  }

  // =================================================================================
  // ========= Удаление ошибок если имеются (2 сек) ==================================
  // =================================================================================
  main_button_last = main_button_read;

  // Проверка длительного нажатия основной кнопки (2 сек)
  if (main_pushed && main_button_state && 
      current_time - main_button_timer >= longPressDelay && 
      !long_press) {
    long_press = true;
    
    // Сброс ошибки Webasto
    if (webasto_fail) {
      webasto_fail = false;
      lcd.setCursor(11, 1);
      lcd.print("    "); // Очистка надписи "FAIL"
    }
  }

  // ============================================================================================
  // ==========================  Очень длинное нажатие (5 секунд) Аварийное выключение ==========
  // ============================================================================================
  if (main_pushed && main_button_state && 
      current_time - main_button_timer >= veryLongPressDelay) {
    if (burn_mode > 0 && burn_mode < 3) {
      burn = false;
      shower = false;
    }
  }

  // ============================================================================
  // Подкачка топлива (10 секунд) - только в режиме выключенного нагревателя
  // ============================================================================
  if (main_pushed && main_button_state && 
      current_time - main_button_timer >= resetPressDelay && 
      burn_mode == 0) {

    const int TOTAL_CYCLES = 200; // Общее количество циклов
    int remaining = TOTAL_CYCLES; // Обратный отсчёт

     // Циклическое включение/выключение топливного насоса
    for (int count = 0; count <= TOTAL_CYCLES; count++) {
      remaining = TOTAL_CYCLES - count; // Вычисляем оставшиеся циклы
      // Однократный вывод сообщения при начале работы
      if (count == 1) {
        lcd.setCursor(0, 1);
        lcd.print("Fuel pumping ");
      }
      // Выводим обратный отсчёт справа (каждые 5 циклов для плавности)
      if (count % 5 == 0 || count == TOTAL_CYCLES) {
        lcd.setCursor(13, 1);
        if (remaining > 99) {
          lcd.print(remaining);
        } else if (remaining > 9) {
          lcd.print(" ");
          lcd.print(remaining);
        } else {
          lcd.print("  ");
          lcd.print(remaining);
        }
      }

      digitalWrite(fuel_pump_pin, HIGH);         // 6герц
      delay(30);                                 // 20% скважность
      digitalWrite(fuel_pump_pin, LOW);
      delay(110);
    }
    // Выводим начальное состояние
    lcd.setCursor(0, 1);
    lcd.print("Complete        ");
    delay(500);
    lcd.setCursor(0, 1);
    lcd.print("OFF             ");
    main_pushed = false; // Сброс после выполнения
  }

  // ===========================================
  // 2. ОБРАБОТКА КНОПКИ ВВЕРХ (PUSHUP)
  // ===========================================
  
  bool pushup_read = digitalRead(pushup_pin) == LOW; // Предполагаем LOW - нажата
  
  if (pushup_read) { // Если кнопка нажата
    if (!pushup_pushed) {
      pushup_pushed = true;
      pushup_timer = current_time;
    }
  } else { // Если кнопка отпущена
    if (pushup_pushed && (current_time - pushup_timer > debounceDelay)) {

      // Разрешаем переключение только если exhaust_temp > 150 или burn == 0
      if (exhaust_temp > 150 || burn == 0) {
        // Переключаем состояние
        switch (currentState) {
          case STATE_HIGH: currentState = STATE_MID; break;
          case STATE_MID: currentState = STATE_LOW; break;
          case STATE_LOW: break; // Остается в STATE_LOW
        }
      }
      pushup_pushed = false;
    }
  }

  // ===========================================
  // 3. ОБРАБОТКА КНОПКИ ВНИЗ (PUSHDOWN)
  // ===========================================
  
  bool pushdown_read = digitalRead(pushdown_pin) == LOW; // Предполагаем LOW - нажата
  
  if (pushdown_read) { // Если кнопка нажата
    if (!pushdown_pushed) {
      pushdown_pushed = true;
      pushdown_timer = current_time;
    }
  } else { // Если кнопка отпущена
    if (pushdown_pushed && (current_time - pushdown_timer > debounceDelay)) {

      // Разрешаем переключение только если exhaust_temp > 150 или burn == 0
      if (exhaust_temp > 150 || burn == 0) {
        // Переключаем состояние
        switch (currentState) {
          case STATE_LOW: currentState = STATE_MID; break;
          case STATE_MID: currentState = STATE_HIGH; break;
          case STATE_HIGH: break; // Остается в STATE_HIGH
        }
      }
      pushdown_pushed = false;
    }
  }
}

// =======================================================================================================
int prev_exhaust_temp = -1;
int prev_fan_speed = -1;
float prev_delayed_period = -1;
int prev_burn_mode = -1;
int prev_debug_glow_plug_on = -1;
String prev_message = (message);
unsigned long fail_blink_timer = 0; // Таймер для мигания надписи "FAIL"
bool fail_displayed = false; // Флаг для отслеживания состояния отображения "FAIL"
int attempt; // Объявление глобальной переменной колличество попыток запуска
int prev_currentState = -1;  // Добавлено для отслеживания изменений systemState


void display_data() {
  // Check if exhaust_temp has changed
  if ((int)exhaust_temp != prev_exhaust_temp) {
    lcd.setCursor(0, 0);
    lcd.print("    ");
    lcd.setCursor(0, 0);
    lcd.print((int)exhaust_temp);
    lcd.print("\xDF"); // Вывод значка градуса после значения температуры
    prev_exhaust_temp = (int)exhaust_temp;
  }

  // Check if fan_speed has changed
  if ((int)fan_speed != prev_fan_speed) {
    lcd.setCursor(5, 0);
    lcd.print(" ");
    lcd.setCursor(5, 0);
    int fillIndex = (fan_speed == 0) ? 7 : map((int)fan_speed, 1, 100, 1, 6);
    lcd.write(fillIndex);

    lcd.setCursor(6, 0);
    lcd.print("    ");
    lcd.setCursor(6, 0);
    lcd.print((int)fan_speed);
    lcd.print("%");
    prev_fan_speed = (int)fan_speed;
  }

  // Check if delayed_period has changed
  float value = 1000.00 / delayed_period;
  if (value != prev_delayed_period || isinf(value) != isinf(prev_delayed_period)) {
    lcd.setCursor(10, 0);
    lcd.print("F:");
    if (isinf(value)) {
      lcd.print("---");
    } else {
      lcd.print(value, 1);
    }
    prev_delayed_period = value;
  }


  // Обновление дисплея только при изменении сообщения
  if (message != prev_message) {
    lcd.setCursor(0, 1);
    lcd.print("                "); // Очистка второй строки
    lcd.setCursor(0, 1);
    lcd.print(message); // Вывод нового сообщения
    prev_message = message; // Обновление предыдущего сообщения

    // Отображение значения ignit_fail при сообщении "Firing Up"
    if (message == "Firing Up") {
      lcd.setCursor(15, 1); // Устанавливаем курсор на правую сторону нижней строки
      lcd.print(attempt);
    }
    // При изменении сообщения сбрасываем предыдущее состояние systemState для принудительного обновления
    prev_currentState = -1;
  }



  // Check if burn_mode has changed
  if (burn_mode != prev_burn_mode) {
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    switch (burn_mode) {
      case 0:
        lcd.print("OFF");
        break;
        
      case 1:
        if(message == "Cooling<70") {
          lcd.print("Cooling<70");
        } else {
        lcd.print("Starting");
        }
        break;
        
      case 2:
        lcd.print("Running");
        break;
      case 3:
        lcd.print("Shuting Down");
        break;
    }
    prev_burn_mode = burn_mode;
    prev_currentState = -1;
  }

    // Check if webasto_fail is 1 and burn_mode is OFF
    if (webasto_fail == 1 && burn_mode == 0) {
        if (millis() - fail_blink_timer >= 500) { // Мигание каждые 500 миллисекунд
            fail_blink_timer = millis();
            fail_displayed = !fail_displayed;

            lcd.setCursor(11, 1); // Устанавливаем курсор на правую сторону нижней строки
            if (fail_displayed) {
                lcd.print("FAIL");
            } else {
                lcd.print("    "); // Очищаем место для надписи "FAIL"
            }
        }
    } else {
        fail_displayed = false;
    }


  // отображаем символ молнии когда свеча включена
  // Проверка изменения debug_glow_plug_on
  if (debug_glow_plug_on != prev_debug_glow_plug_on) {
    lcd.setCursor(15, 0); // Установка курсора на 16-ю позицию первой строки
    if (debug_glow_plug_on == 1) {
      lcd.print(char(244)); // Вывод символа молнии
    } else {
      lcd.print(" "); // Очистка символа
    }
    prev_debug_glow_plug_on = debug_glow_plug_on;
  }

  // Отображение currentState в правой части нижней строки (только при изменении)
  if ((currentState != prev_currentState) && (webasto_fail == 0)) {
    lcd.setCursor(12, 1); // Устанавливаем курсор на правую сторону нижней строки
    lcd.print("    "); // Очищаем область (4 символа)
    lcd.setCursor(12, 1);
    
    switch(currentState) {
      case 0:
        lcd.print("FULL");
        break;
      case 1:
        lcd.print(" MID");
        break;
      case 2:
        lcd.print(" LOW");
        break;
      default:
        lcd.print("    "); // На случай неожиданных значений
    }
    prev_currentState = currentState;
  }
}
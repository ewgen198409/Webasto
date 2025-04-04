#include <math.h> // необходимо для выполнения некоторых вычислений
#include <Arduino.h>
#include <Wire.h> // Библиотека для работы с I2C
#include <LiquidCrystal_I2C.h> // Библиотека для работы с дисплеем
#include <EEPROM.h>

// Укажите адрес вашего I2C дисплея (обычно 0x27 или 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2); // Установите адрес, количество символов и строк

// Переименовываем состояния, чтобы избежать конфликта с Arduino константами
enum SystemState { 
  STATE_HIGH,  // вместо HIGH      // 0
  STATE_MID,   // вместо MID       // 1
  STATE_LOW    // вместо LOW       // 2
};
SystemState currentState = STATE_HIGH;  // Начальное состояние

// Определяем массив для контура знакоместа
byte contour[] = { B11111, B10001, B10001, B10001, B10001, B10001, B10001, B11111 };

byte fill1[8] = { B11111, B10001, B10001, B10001, B10001, B10001, B11111, B11111 }; // 1 и 2 пиксель
byte fill2[8] = { B11111, B10001, B10001, B10001, B10001, B11111, B11111, B11111 }; // 3 пикселя
byte fill3[8] = { B11111, B10001, B10001, B10001, B11111, B11111, B11111, B11111 }; // 4 пикселя
byte fill4[8] = { B11111, B10001, B10001, B11111, B11111, B11111, B11111, B11111 }; // 5 пикселей
byte fill5[8] = { B11111, B10001, B11111, B11111, B11111, B11111, B11111, B11111 }; // 6 пикселей
byte fill6[8] = { B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111 }; // 7 пикселей

// Структура для хранения параметров в EEPROM
struct Settings {
  int pump_size;
  int heater_target;
  int heater_min;
  int heater_overheat;
  int heater_warning;
};

Settings settings;

// Буфер для входящих команд
String inputString = "";
bool stringComplete = false;

// Подключения пинов
int fuel_pump_pin = 5; // Пин для топливного насоса
int glow_plug_pin = 2; // Пин для свечи накаливания
int burn_fan_pin = 9; // Пин для вентилятора горелки
int water_pump_pin = 10; // Пин для водяного насоса
int water_temp_pin = A1; // Пин для датчика температуры воды
int exhaust_temp_pin = A0; // Пин для датчика температуры выхлопа
int push_pin = 13; // Пин для кнопки
int pushup_pin = 12; //    кнопка вверх
int pushdown_pin = 11; //  кнопка вниз
// Подключения пинов

// Конфигурация оборудования
int pump_size = 22;                      // Размер насоса (22, 30, 60)    // код тестировался только на 22, другие разнеры требуют тестов
bool high_temp_exhaust_sensor = true;    // Флаг для высокотемпературного датчика выхлопа
// Конфигурация оборудования

// Топливная смесь
// Предварительная подготовка
int prime_fan_speed = 50; // Скорость вентилятора при предварительной подготовке
int prime_low_temp = 0; // Нижняя температура при предварительной подготовке
int prime_low_temp_fuelrate = 2.5; // Расход топлива при низкой температуре
int prime_high_temp = 20; // Верхняя температура при предварительной подготовке
int prime_high_temp_fuelrate = 2.0; // Расход топлива при высокой температуре

// Начальная
float start_fan_speed = 20; // Начальная скорость вентилятора
float start_fuel = 0.7; // Начальный расход топлива

// Полная мощность
float final_fan_speed = 100.00; // Конечная скорость вентилятора
float final_fuel = 2.38; // Конечный расход топлива    равен 6.5герц при 100% мотора
int full_power_increment_time = 60; // Время увеличения мощности, секунды

// Регулировка мощности
float throttling_high_fuel = 2.20; // Верхний расход топлива при регулировке   ~6герц
float throttling_middle_fuel = 2.01; // средний расход топлива при регулировке    ~5.5герц
float throttling_low_fuel = 1.65; // Нижний расход топлива при регулировке          ~4.5герц

// Топливная смесь

// Настройки последовательного порта
String message = "Off"; // Сообщение для отправки по последовательному порту
bool pushed; // Флаг нажатия кнопки
bool push; // Флаг нажатия кнопки
bool debug_glow_plug_on = 2; // Отладочный флаг для свечи накаливания

// Переменные
float fan_speed; // Скорость вентилятора, проценты
float fuel_need; // Необходимый расход топлива, проценты
int glow_time; // Время накаливания, секунды
float exhaust_temp; // Температура выхлопа, градусы Цельсия
float exhaust_temp_sec[10]; // Массив температур выхлопа за последние 10 секунд, градусы Цельсия
int glow_left = 0; // Оставшееся время накаливания
int last_glow_value = 0; // Последнее значение накаливания
bool burn; // Флаг для горения
bool webasto_fail; // Флаг для сбоя Webasto
bool lean_burn; // Флаг для бедного горения
int delayed_period = 0; // Период задержки
long glowing_on = 0; // Время накаливания
int burn_mode = 0; // Режим горения
// Переменные

// Конфигурация нагревателя
int heater_target = 195; // Целевая температура для нагревателя, градусы Цельсия
int heater_min = 190; // Минимальная температура для нагревателя, градусы Цельсия
int heater_overheat = 210; // Температура перегрева нагревателя, градусы Цельсия
int heater_warning = 200; // Температура предупреждения для нагревателя, градусы Цельсия
// Конфигурация нагревателя

volatile bool settingsUpdateInProgress = false; // Флаг для блокировки logging()

void setup() {

  lcd.init(); // Инициализация дисплея
  lcd.backlight(); // Включение подсветки

  // Создаем пользовательские символы 
  lcd.createChar(7, contour);   // контур

  lcd.createChar(1, fill1);
  lcd.createChar(2, fill2);
  lcd.createChar(3, fill3);
  lcd.createChar(4, fill4);
  lcd.createChar(5, fill5);
  lcd.createChar(6, fill6);

  pinMode(fuel_pump_pin, OUTPUT); // Установка пина топливного насоса как выхода

  // Настройка таймера 2 для glow_plug_pin (170 Гц)
  TCCR2A = 0; // Сброс регистра TCCR2A
  TCCR2B = 0; // Сброс регистра TCCR2B
  TCNT2 = 0; // Сброс счетчика таймера
  OCR2A = 92; // Установка верхнего значения для 170 Гц (16 МГц / 1024 / 170 - 1)
  TCCR2A |= (1 << COM2A1) | (1 << WGM21) | (1 << WGM20); // Настройка режима Fast PWM
  TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20); // Установка предделителя 1024
  pinMode(glow_plug_pin, OUTPUT); // Установка пина свечи накаливания как выхода

  pinMode(burn_fan_pin, OUTPUT); // Установка пина вентилятора горелки как выхода
  pinMode(water_pump_pin, OUTPUT); // Установка пина водяного насоса как выхода
  pinMode(water_temp_pin, INPUT); // Установка пина датчика температуры воды как входа
  pinMode(exhaust_temp_pin, INPUT); // Установка пина датчика температуры выхлопа как входа
  pinMode(push_pin, INPUT); // Установка пина кнопки как входа
  pinMode(pushup_pin, INPUT);  // вход кнопки вверх
  pinMode(pushdown_pin, INPUT);   //  вход кнопки вниз

  Serial.begin(57600); // Инициализация последовательного порта
//  inputString.reserve(94);
  
  EEPROM.get(0, settings);
  
  // Проверка на "первый запуск"
  if (isnan(settings.heater_target) || 
      settings.heater_target < 150 || 
      settings.heater_target > 250) {
    resetToDefaultSettings();
  }
  applySettings();
}

void loop() {
  // Основной цикл работы
  temp_data();
  control();
//  shower_void();
  webasto();
  display_data();

  // Обработка команд от Python-приложения
  processSerialCommands();
  serialEvent();
}

void resetToDefaultSettings() {
  if (sizeof(settings) > EEPROM.length()) {
    Serial.println("ERROR: Settings structure too large for EEPROM");
    return;
  }
  
  settings.pump_size = 22;
  settings.heater_target = 195;
  settings.heater_min = 190;
  settings.heater_overheat = 210;
  settings.heater_warning = 200;
  
  EEPROM.put(0, settings);
}

void applySettings() {
  pump_size = settings.pump_size;
  heater_target = settings.heater_target;
  heater_min = settings.heater_min;
  heater_overheat = settings.heater_overheat;
  heater_warning = settings.heater_warning;
}

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    } else {
      inputString += inChar;
    }
  }
}

void processSerialCommands() {
  if (stringComplete) {
    inputString.trim();
    if (inputString.startsWith("SET:")) {
      handleSettingsUpdate(inputString.substring(4));
    }
    else if (inputString == "UP") {
      handleUpCommand();
    }
    else if (inputString == "DOWN") {
      handleDownCommand();
    }
    else if (inputString == "ENTER") {
      handleEnterCommand();
    }
    else if (inputString == "GET_SETTINGS") {
      sendCurrentSettings();
    }
// При добавлении перестают сохраняться настройки !!!!!
    else if (inputString == "FP") {
      handleFuelPumpingCommand();
    }
    else if (inputString == "CF") {
      // Сброс ошибки Webasto
      if (webasto_fail) {
        webasto_fail = false;
        lcd.setCursor(11, 1);
        lcd.print("    "); // Очистка надписи "FAIL"
        Serial.println("FAIL_CLEARED");
      } else {
        Serial.println("NO_FAIL_TO_CLEAR");
      }
    }
    inputString = "";
    stringComplete = false;
  }
}

void handleUpCommand() {
  if (exhaust_temp > 150 || burn == 0) {
    switch (currentState) {
      case STATE_HIGH: currentState = STATE_MID; break;
      case STATE_MID: currentState = STATE_LOW; break;
      case STATE_LOW: break;
    }
  }
  Serial.println("State: " + String(currentState));
}

void handleDownCommand() {
  if (exhaust_temp > 150 || burn == 0) {
    switch (currentState) {
      case STATE_LOW: currentState = STATE_MID; break;
      case STATE_MID: currentState = STATE_HIGH; break;
      case STATE_HIGH: break;
    }
  }
  Serial.println("State: " + String(currentState));
}

void handleEnterCommand() {
  if (burn_mode == 1 || burn_mode == 2) {
    burn = false;
  } else {
    burn = true;
  }
  Serial.println("Burn: " + String(burn ? "ON" : "OFF"));
}

void sendCurrentSettings() {
  Serial.print("CURRENT_SETTINGS:");
  Serial.print("pump_size="); Serial.print(settings.pump_size); Serial.print(",");
  Serial.print("heater_target="); Serial.print(settings.heater_target, 1); Serial.print(",");
  Serial.print("heater_min="); Serial.print(settings.heater_min, 1); Serial.print(",");
  Serial.print("heater_overheat="); Serial.print(settings.heater_overheat, 1); Serial.print(",");
  Serial.print("heater_warning="); Serial.println(settings.heater_warning, 1);
}

void handleSettingsUpdate(String paramsStr) {
  settingsUpdateInProgress = true; // Блокируем logging()
  
  
  int paramsFound = 0;
  while (paramsStr.length() > 0) {
    int separatorPos = paramsStr.indexOf(',');
    String param = (separatorPos == -1) ? paramsStr : paramsStr.substring(0, separatorPos);
    param.trim(); // Удаляем пробелы вокруг параметра
    
    int eqPos = param.indexOf('=');
    if (eqPos != -1) {
      String key = param.substring(0, eqPos);
      key.trim(); // Удаляем пробелы у ключа
      
      String value = param.substring(eqPos + 1);
      value.trim(); // Удаляем пробелы у значения
      
      int intValue = value.toInt();
      if (intValue != 0 || value.equals("0")) {
        if (key.equals("pump_size")) {
          settings.pump_size = intValue;
          paramsFound++;
        }
        else if (key.equals("heater_target")) {
          settings.heater_target = intValue;
          paramsFound++;
        }
        else if (key.equals("heater_min")) {
          settings.heater_min = intValue;
          paramsFound++;
        }
        else if (key.equals("heater_overheat")) {
          settings.heater_overheat = intValue;
          paramsFound++;
        }
        else if (key.equals("heater_warning")) {
          settings.heater_warning = intValue;
          paramsFound++;
        } else {
          Serial.print("Unknown key: ");
          Serial.println(key);
        }
      } else {
        Serial.print("Invalid value for key '");
        Serial.print(key);
        Serial.print("': ");
        Serial.println(value);
      }
    }
    
    if (separatorPos == -1) break;
    paramsStr = paramsStr.substring(separatorPos + 1);
  }
  
  if (paramsFound > 0) {
    EEPROM.put(0, settings);
    applySettings();
    Serial.println("SETTINGS_OK");
    settingsUpdateInProgress = false; // Разблокируем logging(
  } else {
    Serial.println("SETTINGS_ERROR");
    settingsUpdateInProgress = false; // Разблокируем logging(
  }
}
//
// Обработка команды прокачки топлива (аналог удержания кнопки 10 секунд)
void handleFuelPumpingCommand() {
  if (burn_mode == 0) { // Только если нагреватель выключен
    const int TOTAL_CYCLES = 200; // Общее количество циклов
    int remaining = TOTAL_CYCLES; // Обратный отсчёт

    // Циклическое включение/выключение топливного насоса
    for (int count = 0; count <= TOTAL_CYCLES; count++) {
      remaining = TOTAL_CYCLES - count;
      
      // Однократный вывод сообщения при начале работы
      if (count == 1) {
        lcd.setCursor(0, 1);
        lcd.print("Fuel pumping");
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

      digitalWrite(fuel_pump_pin, HIGH); // 6герц
      delay(30);                        // 20% скважность
      digitalWrite(fuel_pump_pin, LOW);
      delay(110);
    }
    
    // Выводим начальное состояние
    lcd.setCursor(0, 1);
    lcd.print("Complete        ");
    delay(500);
    lcd.setCursor(0, 1);
    lcd.print("OFF             ");
  } 
}

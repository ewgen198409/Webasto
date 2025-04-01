void water_pump() {
  // Преобразует скорость насоса из процентов в значение от 0 до 255
  int percent_map = mapf(water_pump_speed, 0, 100, 0, 255);
  static bool overheat; // Статическая переменная для отслеживания перегрева

  // Если происходит серьезный перегрев
  if (exhaust_temp > 400 || water_temp > 150 || (overheat && water_temp > 150)) {
    overheat = 1; // Устанавливаем флаг перегрева
    percent_map = 255; // Устанавливаем максимальную скорость насоса
    webasto_fail = 1; // Устанавливаем флаг неисправности Webasto
  } else if (overheat && water_temp <= water_warning) {
    overheat = 0; // Снимаем флаг перегрева
    percent_map = 0; // Устанавливаем минимальную скорость насоса
  }

  // Условия для различных режимов горения
  if(burn_mode >= 1 && burn_mode <= 3 && water_temp > water_warning)
    percent_map = 255; // Устанавливаем максимальную скорость насоса
  else if(burn_mode == 1 && water_temp > 15)
    percent_map = 232; // Устанавливаем скорость насоса на 232

  // Логика для отслеживания запуска насоса
  if(water_pump_started == 0 && water_pump_speed > 0) {
    water_pump_started_on = millis(); // Запоминаем время запуска насоса
    water_pump_started = 1; // Устанавливаем флаг запуска насоса
  } else if(water_pump_speed == 0) {
    water_pump_started = 0; // Снимаем флаг запуска насоса
  }

  // Устанавливаем максимальную скорость насоса в течение первых 2 секунд после запуска
  if(water_pump_started == 1 && (millis() - water_pump_started_on) <= 2000)
    percent_map = 255;
  else
    water_pump_started_on = 0; // Сбрасываем время запуска

  debug_water_percent_map = percent_map; // Сохраняем значение для отладки

  // Устанавливаем скорость насоса на выходной пин
  analogWrite(water_pump_pin, percent_map);
}

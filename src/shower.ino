// Функция, регулирующая поток насоса
void shower_void() {
  static unsigned long secs_timer; // Статическая переменная для хранения времени в миллисекундах

  // Проверка и обновление таймера
  if (millis() < secs_timer) {
    secs_timer = millis();
  }

  static int seconds; // Статическая переменная для хранения количества секунд

  if (shower) { // Если душ включен
    burn = 1; // Устанавливаем режим горения

    // Проверка условий для изменения скорости насоса
    if (burn_mode != 1 && (exhaust_temp > exhaust_temp_sec[9] + 1 || exhaust_temp > 30)) {
      // Если температура выхлопа превышает заданные значения

      if (water_temp < shower_min) {
        water_pump_speed = 91; // Устанавливаем скорость насоса на 91%
        // Начинаем медленно циркулировать воду, чтобы предотвратить её закипание внутри системы
      } else if (water_temp - shower_target > -2) {
        // Если температура воды близка к целевой
        water_pump_speed = mapf(water_temp, shower_target - 1, shower_target + 5, 91, 100);
        // Функция mapf регулирует поток насоса в диапазоне от 10% до 60% в зависимости от того, насколько температура воды близка к целевой
      } else if (water_temp - shower_target > 0.5) {
        // Если вода начинает перегреваться
        water_pump_speed = mapf(water_temp, shower_target + 1, shower_target + 5, 91, 100);
        // Это предотвращает закипание воды внутри системы
      }
    } else {
      water_pump_speed = 0; // Выключаем насос
    }

    // Если режим горения включен и температура воды превышает предупреждающее значение
    if(burn_mode == 1 && water_temp > water_warning)
      water_pump_speed = 100; // Устанавливаем максимальную скорость насоса
  // } else {
  //   // Реинициализация переменных, если душ выключен
  //   burn = 0;
  //   water_pump_speed = 0;
  //   seconds = 0;
  //   shower_timeout = 0;
  //   lean_burn = 0;
  }

  water_pump(); // Вызов функции управления насосом
}

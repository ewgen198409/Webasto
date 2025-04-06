void glow_plug() {
    // Эта функция управляет работой свечи накаливания.
    // Розжиг свечи в течение 10сек
    // Затухание свечи в течение 5сек
    // Если время накала (glow_time) больше 0, свеча включается, и время накала уменьшается каждую секунду.

    static long timer; // Статическая переменная для хранения времени таймера
    static bool is_fading_out = false; // Флаг для отслеживания процесса плавного выключения
    static unsigned long fade_out_start = 0; // Время начала плавного выключения

    if(glow_left < 0)
        glow_time = 0; // Если оставшееся время накала меньше 0, сбрасываем время накала

    if(last_glow_value != glow_time) {
        // Если значение времени накала изменилось
        glow_left = glow_time; // Обновляем оставшееся время накала
        last_glow_value = glow_time; // Обновляем последнее значение времени накала

        if(glow_time != 0) {
            timer = millis(); // Если время накала не равно 0, обновляем таймер
            is_fading_out = false; // Сброс флага плавного выключения при новом цикле накала
        }
    }

    if(glow_time == 0)
        timer = millis(); // Если время накала равно 0, обновляем таймер

    if(millis() - timer >= 1000) {
        // Если прошла одна секунда
        timer = millis(); // Обновляем таймер
        glow_left -= 1; // Уменьшаем оставшееся время накала на 1 секунду
    }

    if(glow_left > 0) {
        // Если оставшееся время накала больше 0
        if(glowing_on == 0)
            glowing_on = millis(); // Если свеча не горит, запоминаем время начала накала

        // Если свеча горит
        if (glowing_on != 0) {
            unsigned long elapsed_time = millis() - glowing_on;

            // Если прошло менее 10 секунд с момента начала накала
            if (elapsed_time < 10000) {
                int brightness = map(elapsed_time, 0, 10000, 0, 461); // Линейная интерполяция яркости | maximum ~45%
                //                                    для 328PB 115!!!
                analogWrite(glow_plug_pin, brightness); // Постепенно увеличиваем яркость свечи
            } else {
                analogWrite(glow_plug_pin, 461); // Устанавливаем максимальную яркость свечи
                //               для 328PB 115!!!
            }
        }
        debug_glow_plug_on = 1; // Устанавливаем отладочный флаг, что свеча горит
        is_fading_out = false; // Сброс флага плавного выключения, так как свеча ещё горит
    } else {
        // Если оставшееся время накала равно 0
        if (glowing_on != 0 && !is_fading_out) {
            // Если свеча горела и ещё не начато плавное выключение
            is_fading_out = true;
            fade_out_start = millis();
            glowing_on = 0; // Сбрасываем время начала накала
        }

        if (is_fading_out) {
            unsigned long fade_time = millis() - fade_out_start;
            
            if (fade_time < 5000) {
                // Плавное уменьшение яркости в течение 10 секунд
                int brightness = map(fade_time, 0, 5000, 461, 0);
                //                             для 328PB 115!!!
                analogWrite(glow_plug_pin, brightness);
            } else {
                // Завершение плавного выключения
                analogWrite(glow_plug_pin, 0); // Полностью выключаем свечу
                debug_glow_plug_on = 0; // Сбрасываем отладочный флаг
                glow_time = 0; // Сбрасываем время накала
                is_fading_out = false; // Сбрасываем флаг плавного выключения
            }
        } else {
            // Если свеча уже выключена
            analogWrite(glow_plug_pin, 0); // Выключаем свечу
            debug_glow_plug_on = 0; // Сбрасываем отладочный флаг
            glow_time = 0; // Сбрасываем время накала
        }
    }
}

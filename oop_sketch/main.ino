#include "Core.h"

Core* core;
// Переменные для энкодера
volatile int encCount = 0;      // Счетчик импульсов энкодера
volatile boolean state0;        // Текущее состояние
volatile boolean lastState;     // Предыдущее состояние
volatile boolean turnFlag;      // Флаг поворота энкодера


/**
 * @brief Инициализация системы.
 */
void setup() {
    // Настройка таймеров (внешняя функция)
    setupTmr();
    core = new Core();
}

void loop() {
    core->loop(encCount);
}

void encTick() {
    bool currentState = digitalRead(encoderPinA); // Чтение текущего состояния пина A энкодера
    if (currentState != lastState) { // Проверка изменения состояния
        // В зависимости от типа энкодера, применяем разную логику
        if (ENC_TYPE == 1) {
            turnFlag = !turnFlag; // Инвертируем флаг при каждом изменении
            if (turnFlag) {
                // Увеличиваем или уменьшаем счетчик в зависимости от состояния пина B
                encCount += (digitalRead(encoderPinB) != lastState) ? -1 : 1;
            }
        } else {
            // Для другого типа энкодера - просто корректируем счетчик
            encCount += (digitalRead(encoderPinB) != lastState) ? -1 : 1;
        }
        lastState = currentState; // Обновляем предыдущее состояние
    }
}


// опрос энкодера в прерывании
ISR(TIMER3_COMPA_vect) {
  encTick();
}

// ставим таймер 3, канал А, период 0.5 мс. Для опроса энкодера
void setupTmr() {
  TCCR3B = 0b00001001;
  TIMSK3 = 0b00000010;
  OCR3AH = highByte(15999 / 2);
  OCR3AL = lowByte(15999 / 2);
}

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <GyverButton.h>
#include <HID-Project.h>
#include <EEPROM.h>

///<=====================НАСТРОЙКИ====================>
#define ENC_TYPE 1                  /// тип энкодера, 0 или 1

#define encoderPinA 5               /// пин A энкодера
#define encoderPinB 7               /// пин B энкодера

#define pinGas 10                   /// пин Газа
#define pinBrake 9                 /// пин тормоза
#define pinClutch 6                /// пин Сцепления

#define button1Pin 15               /// пин Кнопки1
#define button2Pin 14               /// пин Кнопки2
#define button3Pin 16               /// пин Кнопки3
///<=====================НАСТРОЙКИ====================>
// Настройки экрана OLED
#define SCREEN_WIDTH 128   // Ширина экрана в пикселях
#define SCREEN_HEIGHT 64   // Высота экрана в пикселях
#define OLED_RESET -1      // Пин сброса OLED (не используется)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Указатели на кнопки
GButton* button1Up = nullptr;  // Кнопка "Вверх"
GButton* button2Down = nullptr; // Кнопка "Вниз"
GButton* button3ok = nullptr;  // Кнопка "OK"

// Переменные состояния
int CursorPos = 0;     // Позиция курсора на экране
int pedalGas = 0;      // Значение газа
int pedalBrake = 0;    // Значение тормоза
int pedalClutch = 0;   // Значение сцепления
int wheel = 0;         // Угол поворота руля
int HShifter = 0;      // Состояние H-образного переключателя передач
int gear = 0;          // Текущая передача
int gas = 0, brake, clutch;
int RawValueGas = 0, RawValueBrake = 0, RawValueClutch = 0; // Значения педалей в режиме Raw
int resetValueGas = 0;


// Переменные для энкодера
volatile int encCount = 0;      // Счетчик импульсов энкодера
volatile boolean state0;        // Текущее состояние
volatile boolean lastState;     // Предыдущее состояние
volatile boolean turnFlag;      // Флаг поворота энкодера

// Переменные для управления обновлением
int lastTimeupdate = 0;         // Время последнего обновления
bool forceUpdate = false;       // Принудительное обновление экрана

// Настройки руля
int wheelValues[] = {90, 180, 360, 540, 720, 900, 1080}; // Возможные значения углов
int currentWheelIndex = 0;       // Индекс текущего значения из массива
int wheelDeg = wheelValues[currentWheelIndex]; // Начальное значение угла руля

// Калибровочные значения для педалей
int pedalAdjustmentGasMin = 0, pedalAdjustmentGasMax = 0;        // Газ
int pedalAdjustmentBrakeMin = 0, pedalAdjustmentBrakeMax = 0;    // Тормоз
int pedalAdjustmentClutchMin = 0, pedalAdjustmentClutchMax = 0;  // Сцепление

// Статусы и флаги
int STATUS = 0;                    // Текущий статус системы
int miniStatus = 0;          // Статус калибровки педалей
int microStatus = 0;       // Статус нижнего значения калибровки
bool resetStatusAdjustment = false; // Флаг сброса статусов калибровки
bool update = false;               // Флаг обновления состояния
bool updateFirmware = false;               // Флаг обновления прошивки
bool readflag = false;             // Флаг чтения данных
bool modeZeroToMax = false;        // Флаг инверсии педалей

// Диапазоны педалей (минимальные и максимальные значения)
int gasMin = 0, gasMax = 1023;        // Диапазон газа
int brakeMin = 0, brakeMax = 1023;    // Диапазон тормоза
int clutchMin = 0, clutchMax = 1023;  // Диапазон сцепления



/**
 * @brief Инициализация системы.
 * 
 * Инициализирует дисплей, кнопки и серийный порт. 
 * Устанавливает параметры антидребезга, таймаутов удержания и между кликами для кнопок. 
 * Выводит стартовое сообщение на дисплей и выполняет настройку начального состояния системы.
 */
void setup() {
    // Настройка таймеров (внешняя функция)
    setupTmr();

    // Инициализация OLED-дисплея
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Инициализация с I2C-адресом 0x3C
    display.clearDisplay();                   // Очистка экрана
    display.setTextColor(WHITE);              // Установка цвета текста (белый)
    display.display();                        // Применение изменений на экране

    // Инициализация кнопок
    button1Up = new GButton(button1Pin);       // Кнопка "Вверх"
    button2Down = new GButton(button2Pin);     // Кнопка "Вниз"
    button3ok = new GButton(button3Pin);       // Кнопка "ОК"

    // Настройка параметров кнопки "Вверх"
    button1Up->setDebounce(20);                // Антидребезг: 20 мс
    button1Up->setTimeout(500);                // Таймаут удержания: 500 мс
    button1Up->setClickTimeout(100);           // Таймаут между кликами: 100 мс

    // Настройка параметров кнопки "Вниз"
    button2Down->setDebounce(20);              // Антидребезг: 20 мс
    button2Down->setTimeout(500);              // Таймаут удержания: 500 мс
    button2Down->setClickTimeout(100);         // Таймаут между кликами: 100 мс

    // Настройка параметров кнопки "ОК"
    button3ok->setDebounce(50);                // Антидребезг: 50 мс
    button3ok->setTimeout(1000);               // Таймаут удержания: 1000 мс
    button3ok->setClickTimeout(300);           // Таймаут между кликами: 300 мс

    // Инициализация серийного порта и игрового контроллера
    Serial.begin(9600);                        // Скорость обмена по Serial: 9600 бод
    Gamepad.begin();                           // Инициализация игрового контроллера
    loadFunc();
    // Установка начального состояния системы
    home();                                    // Переход в главное меню или начальную позицию
}

/**
 * @brief Основной цикл программы.
 * 
 * Функция вызывается многократно в процессе работы программы. 
 * Она:
 * 1. Обрабатывает события кнопок через метод `tick()`.
 * 2. Проверяет наличие щелчков или обновлений.
 * 3. Вызывает соответствующую функцию на основе текущего значения `STATUS`.
 * 4. Проверяет необходимость обновления экрана.
 * 5. Выполняет чтение данных, если установлен флаг `readflag`.
 */
void loop() {
    if (!updateFirmware){
        // Обработка кнопок
        button1Up->tick();
        button2Down->tick();
        button3ok->tick();

        // Проверка нажатий кнопок или флага обновления
        if (button1Up->hasClicks() || button2Down->hasClicks() || button3ok->hasClicks() || update) {
            update = false; // Сброс флага обновления

            // Выбор действия в зависимости от текущего статуса
            switch (STATUS) {
                case 0: home(); break;                // Главный экран
                case 1: mainFunc(); break;            // Основная функция
                case 2: callibrationFunc(); break;    // Функция калибровки
                case 3: adjustmentFunc(); break;      // Настройки
                case 4: debugFunc(); break;           // Отладка
                case 5: saveFunc(); break;            // Сохранение
                default: break;                       // Неизвестный статус (ничего не делаем)
            }
        }

        // Обновление экрана, если прошло более 300 мс и установлен флаг `forceUpdate`
        if (millis() - lastTimeupdate > 1000 && forceUpdate) {
            lastTimeupdate = millis(); // Обновление времени последнего обновления
            update = true;             // Установка флага обновления
        }

        // Обработка чтения данных, если установлен флаг
        if (readflag) {
            read(); // Вызов функции чтения данных
        }
        // Считывание аналоговых значений педалей
        RawValueGas = analogRead(pinGas);
        RawValueBrake = analogRead(pinBrake);
        RawValueClutch = analogRead(pinClutch);
    }
}



/**
 * @brief Считывание и обработка аналоговых входов для руля и педалей.
 * 
 * Функция:
 * 1. Считывает значение энкодера для руля и отображает его в заданном диапазоне.
 * 2. Считывает аналоговые значения педалей (газ, тормоз, сцепление) и корректирует их 
 *    на основе настроек (минимум, максимум, калибровочные значения).
 * 3. Обрабатывает инверсию педалей, если включен флаг `INVERTEDPEDAL`.
 * 4. Передает обработанные значения в игровой контроллер `Gamepad`.
 */
void read() {
    // Считывание текущего значения энкодера (руля)
    // Считывание текущего значения энкодера (руля)
    int wheelValue = encCount;
    if (modeZeroToMax) {
        // Режим от 0 до макс
        wheelValue = constrain(wheelValue, 0, wheelDeg / 2); // Ограничиваем от 0 до половины диапазона
        wheel = map(wheelValue, 0, wheelDeg / 2, 0, 32767);  // Преобразование в диапазон [0, 32767]
    } else {
        // Режим -макс до макс
        wheelValue = constrain(wheelValue, -wheelDeg / 4, wheelDeg / 4); // Ограничиваем значение
        wheel = map(wheelValue, -wheelDeg / 4, wheelDeg / 4, -32768, 32767); // Преобразование
    }

    // Корректировка значений педалей
    if (modeZeroToMax) {
        // Режим от 0 до макс
        gas = constrain(map(RawValueGas, gasMin - pedalAdjustmentGasMin, gasMax + pedalAdjustmentGasMax, 0, 32767), 0, 32767);
        brake = constrain(map(RawValueBrake, brakeMin - pedalAdjustmentBrakeMin, brakeMax + pedalAdjustmentBrakeMax, 0, 127), 0, 127);
        clutch = constrain(map(RawValueClutch, clutchMin - pedalAdjustmentClutchMin, clutchMax + pedalAdjustmentClutchMax, 0, 127), 0, 127);
    } else {
        // Режим -макс до макс
        gas = constrain(map(RawValueGas, gasMin - pedalAdjustmentGasMin, gasMax + pedalAdjustmentGasMax, -32765, 32765), -32768, 32767);
        brake = constrain(map(RawValueBrake, brakeMin - pedalAdjustmentBrakeMin, brakeMax + pedalAdjustmentBrakeMax, -128, 127), -128, 127);
        clutch = constrain(map(RawValueClutch, clutchMin - pedalAdjustmentClutchMin, clutchMax + pedalAdjustmentClutchMax, -128, 127), -128, 127);
    }

    Serial.print("gas: "); Serial.print(gas); Serial.print(" brake: "); Serial.print(brake); Serial.print(" clutch: "); Serial.println(clutch);
    

    // Передача значений в игровой контроллер
    Gamepad.xAxis(wheel);
    Gamepad.yAxis(gas);
    Gamepad.zAxis(brake);
    Gamepad.rzAxis(clutch);

    // Отправка данных игровому контроллеру
    Gamepad.write();
}


/**
 * @brief Главное меню устройства.
 *
 * Функция отображает меню с пятью опциями:
 * 1. MAIN - Главная функция
 * 2. CALLIBRATION - Калибровка устройства
 * 3. ADJUSTMENT - Настройка параметров
 * 4. DEBUG - Режим отладки
 * 5. SAVE - Сохранение данных
 *
 * Пользователь может:
 * - Использовать кнопки Вверх и Вниз для навигации.
 * - Нажать кнопку "OK" для выбора опции.
 * 
 * При выборе опции вызывается соответствующая функция, 
 * а переменная `STATUS` обновляется для отслеживания текущего состояния.
 */
void home() {
    // Очистка дисплея перед отрисовкой нового меню
    display.clearDisplay();

    // Массив пунктов меню
    const char* menuItems[] = {
        "  MAIN",         // Главная функция
        "  CALLIBRATION", // Калибровка
        "  ADJUSTMENT",   // Настройка
        "  DEBUG",        // Отладка
        "  SAVE"          // Сохранение
    };

    // Отрисовка всех пунктов меню
    for (int i = 0; i < 5; i++) {
        display.setCursor(0, i * 9); // Установка позиции курсора для строки
        display.print(menuItems[i]); // Вывод текста меню
    }

    // Управление перемещением курсора
    if (button2Down->isSingle()) { 
        CursorPos = (CursorPos + 1) % 5; // Перемещение вниз (циклически)
    }
    if (button1Up->isSingle()) { 
        CursorPos = (CursorPos - 1 + 5) % 5; // Перемещение вверх (циклически)
    }

    // Обработка выбора пункта меню
    if (button3ok->isSingle()) { 
        STATUS = CursorPos + 1; // Установка статуса в зависимости от позиции курсора

        if (STATUS == 3) { // Если выбрана настройка
            miniStatus = -1; // Сброс состояния настройки
            CursorPos = 0; // Сброс позиции курсора
        }

        if (STATUS == 2) { // Если выбран режим отладки
            miniStatus = -1; // Сброс состояния настройки
            CursorPos = 0; // Сброс позиции курсора
        }
    }

    // Вызов соответствующей функции на основе текущего статуса
    switch (STATUS) {
        case 1: mainFunc(); forceUpdate = false; break;        // Главная функция
        case 2: callibrationFunc(); forceUpdate = false; break; // Калибровка
        case 3: adjustmentFunc(); forceUpdate = false; break;  // Настройка
        case 4: debugFunc(); forceUpdate = true; break;       // Отладка
        case 5: saveFunc(); forceUpdate = false; break;       // Сохранение
        default: break; // Ничего не делать, если статус не соответствует пункту меню
    }

    // Отрисовка текущей позиции курсора
    drawCursor();

    // Обновление дисплея для применения изменений
    display.display();
}

/**
 * @brief Отображает курсор в текущей позиции.
 *
 * Функция используется для отрисовки стрелки ">" рядом с пунктом меню, на который указывает `CursorPos`.
 */
void drawCursor() {
    display.setCursor(0, CursorPos * 9); // Установка позиции курсора в строку, соответствующую `CursorPos`
    display.print("> "); // Отрисовка курсора
}

/**
 * @brief Отображает указатель выбора на указанной строке.
 *
 * Отрисовывает стрелку "<=" в правой части дисплея, указывая на выбранный пункт.
 *
 * @param line Номер строки, где будет отрисован указатель.
 */
void drawCursorSelect(int line) {
    // display.setCursor(110, line * 9); // Установка позиции для отображения в правой части строки
    // display.print("<="); // Отрисовка указателя
    display.setCursor(0, CursorPos * 9); // Установка позиции курсора в строку, соответствующую `CursorPos`
    display.print(" >"); // Отрисовка курсора
}

/**
 * @brief Главная функция программы.
 *
 * Отображает текущие значения педалей (газ, тормоз, сцепление) и руля. 
 * Обрабатывает двойное нажатие кнопки "OK" для возврата в главное меню.
 */
void mainFunc() {
    display.clearDisplay(); // Очистка дисплея перед обновлением

    // Проверка на двойное нажатие кнопки "OK" для возврата в главное меню
    if (button3ok->isDouble()) {
        resetStatusAdjustment = false; // Сброс флага настройки
        miniStatus = 0;          // Сброс состояния настройки
        CursorPos = 0;                 // Сброс курсора
        STATUS = 0;                    // Возврат в главное меню
        update = true;                 // Установка флага обновления
        return;
    }
    if (button3ok->isSingle()) {
        encCount = 0; // Сброс счетчика рулей
    }

    // Получение текущего значения руля
    wheel = encCount * 4;

    // Отображение значений педалей и руля
    display.setCursor(0, 0);
    display.print("  GAS:        ");
    display.print(pedalGas);

    display.setCursor(0, 9);
    display.print("  BRAKE:      ");
    display.print(pedalBrake);

    display.setCursor(0, 18);
    display.print("  CLUTCH:     ");
    display.print(pedalClutch);

    display.setCursor(0, 27);
    display.print("  WHEEL:      ");
    display.print(wheel);

    display.display(); // Обновление дисплея
}

/**
 * @brief Функция калибровки (пока пустая).
 *
 * Здесь будет размещен код для калибровки устройства.
 */
void callibrationFunc() {
    display.clearDisplay();

    // Обработка состояния
    switch (miniStatus) {
        case 0: { // Основной режим выбора
            handleCursorMovement(2); // Обработка перемещения курсора в пределах [0-3]

            if (button3ok->isSingle()) {
                miniStatus = CursorPos + 1; // Переход к выбранному состоянию
            }
            if (button3ok->isDouble()) { // Выход
                resetToMainStatus();
                return;
            }
            break;
        }
        case 1: // GAS
            if (microStatus == 0){
                if (button3ok->isSingle()) {
                    gasMax = RawValueGas;
                };
            }
            else{
                if (button3ok->isSingle()) {
                    gasMin = RawValueGas;
                };
            }
            resetStatusAdjustment = true;
            break;
        case 2: // BRAKE
            if (microStatus == 0){
                if (button3ok->isSingle()) {
                    brakeMax = RawValueBrake;
                };
            }
            else{
                if (button3ok->isSingle()) {
                    brakeMin = RawValueBrake;
                };
            }
            resetStatusAdjustment = true;
            break;
        case 3: // CLUTCH
            if (microStatus == 0){
                if (button3ok->isSingle()) {
                    clutchMax = RawValueClutch;
                };
            }
            else{
                if (button3ok->isSingle()) {
                    clutchMin = RawValueClutch;
                };
            }
            resetStatusAdjustment = true;
            break;
        default:
            miniStatus = 0; // Безопасное состояние
            break;
    }
    

    // Возврат к основному режиму
    if ((miniStatus == 1 || miniStatus == 2 || miniStatus == 3) && resetStatusAdjustment) {
        if (button3ok->isSingle()) {
            if (microStatus == 0){
                microStatus = 1;
            } else {
                microStatus = 0;
                miniStatus = 0;
            }      
            resetStatusAdjustment = false;
        }
    }
    // Возврат к основному режиму
    if ((miniStatus == 4 || miniStatus == 5) && resetStatusAdjustment) {
        if (button3ok->isSingle()) {
            microStatus = 0;
            miniStatus = 0;
            resetStatusAdjustment = false;
        }
    }

    // Отрисовка данных на экране
    displayMenuDataCallibration();
    drawCursor();
    if (miniStatus != 0) {
        drawCursorSelect(CursorPos);
    }

    display.display();
}


/**
*Функция регулировки
*
* Функция, которая управляет регулировкой деталей отделки короля
*
* @return void
*/
void adjustmentFunc() {
    display.clearDisplay();

    // Обработка состояния
    switch (miniStatus) {
        case 0: { // Основной режим выбора
            handleCursorMovement(6); // Обработка перемещения курсора в пределах [0-3]

            if (button3ok->isSingle()) {
                miniStatus = CursorPos + 1; // Переход к выбранному состоянию
            }
            if (button3ok->isDouble()) { // Выход
                resetToMainStatus();
                return;
            }
            break;
        }
        case 1: // GAS
            if (microStatus == 0)
                processPedalAdjustment(pedalAdjustmentGasMax);
            else
                processPedalAdjustment(pedalAdjustmentGasMin);
            break;
        case 2: // BRAKE
            if (microStatus == 0)
                processPedalAdjustment(pedalAdjustmentBrakeMax);
            else
                processPedalAdjustment(pedalAdjustmentBrakeMin);
            break;
        case 3: // CLUTCH
            if (microStatus == 0)
                processPedalAdjustment(pedalAdjustmentClutchMax);
            else
                processPedalAdjustment(pedalAdjustmentClutchMin);
            break;
        case 4: // WHEEL
            processWheelAdjustment();
            break;
        case 5: // Gamepad readflag
            readflag = !readflag;
            miniStatus = 0;
            break;
        case 6: // modeZeroToMax
            modeZeroToMax = !modeZeroToMax;
            miniStatus = 0;
            break;
        case 7: // updateFirmware
            updateFirmware = !updateFirmware;
            break;
        default:
            miniStatus = 0; // Безопасное состояние
            break;
    }
    

    // Возврат к основному режиму
    if ((miniStatus == 1 || miniStatus == 2 || miniStatus == 3) && resetStatusAdjustment) {
        if (button3ok->isSingle()) {
            if (microStatus == 0){
                microStatus = 1;
            } else {
                microStatus = 0;
                miniStatus = 0;
            }      
            resetStatusAdjustment = false;
        }
    }
    // Возврат к основному режиму
    if ((miniStatus == 4 || miniStatus == 5) && resetStatusAdjustment) {
        if (button3ok->isSingle()) {
            microStatus = 0;
            miniStatus = 0;
            resetStatusAdjustment = false;
        }
    }

    // Отрисовка данных на экране
    displayMenuData();
    drawCursor();
    if (miniStatus != 0) {
        drawCursorSelect(CursorPos);
    }

    display.display();
}

/**
*Обработка перемещения курсора
*
* Функция, которая изменяет текущий индекс курсора, обработавая события
* нажатия кнопок "Вниз" и "Вверх". Индекс курсора изменяется в пределах
* [0, maxIndex] циклически.
*
* @param maxIndex - максимальный индекс курсора
*/
void handleCursorMovement(int maxIndex) {
    if (button2Down->isSingle()) {
        CursorPos = (CursorPos + 1) % (maxIndex + 1); // Циклический переход
    }
    if (button1Up->isSingle()) {
        CursorPos = (CursorPos - 1 + (maxIndex + 1)) % (maxIndex + 1); // Циклический переход
    }
}

/** 
* @brief Сбрасывает состояние системы в основное состояние.
*
* Эта функция сбрасывает несколько глобальных переменных в их начальное состояние,
* устанавливая STATUS в 0, сбрасывая позицию курсора в 0 и отмечая
* статус настройки как -1. Она используется для возврата системы
* в ее основное состояние из любого другого состояния.
*/
void resetToMainStatus() {
    STATUS = 0;
    CursorPos = 0;
    miniStatus = -1;
    microStatus = 0;
    update = true;

}

/**
* @brief Обработка настройки педали
*
* Функция, которая изменяет переданное значение настройки педали,
* используя функцию adjustmentFunc, и отмечает, что состояние
* настройки было изменено.
*
* @param pedalAdjustmentValue - значение настройки педали
*/
void processPedalAdjustment(int& pedalAdjustmentValue) {
    pedalAdjustmentValue = adjustmentFunc(pedalAdjustmentValue);
    resetStatusAdjustment = true;
}


/**
* @brief Обработка настройки колеса
*
* Функция, которая изменяет выбранное значение колеса,
* используя кнопки 1 и 2, и отмечает, что состояние
* настройки было изменено.
*/
void processWheelAdjustment() {
    if (button2Down->isSingle()) {
        currentWheelIndex = (currentWheelIndex + 1) % (sizeof(wheelValues) / sizeof(wheelValues[0]));
    }
    if (button1Up->isSingle()) {
        currentWheelIndex = (currentWheelIndex == 0)
                            ? (sizeof(wheelValues) / sizeof(wheelValues[0])) - 1
                            : currentWheelIndex - 1;
    }
    wheelDeg = wheelValues[currentWheelIndex];
    resetStatusAdjustment = true;
}


/**
* @brief Вывод меню настройки
*
* Функция выводит на дисплей текущие значения настройки
* педалей газа, тормоза и сцепления, а также выбранное
* значение колеса.
*/
void displayMenuData() {
    display.setCursor(0, 0);
    display.print("  GAS:        ");
    display.print(pedalAdjustmentGasMax);
    display.print(" ");
    display.print(pedalAdjustmentGasMin);
    display.setCursor(0, 9);
    display.print("  BRAKE:      ");
    display.print(pedalAdjustmentBrakeMax);
    display.print(" ");
    display.print(pedalAdjustmentBrakeMin);
    display.setCursor(0, 18);
    display.print("  CLUTCH:     ");
    display.print(pedalAdjustmentClutchMax);
    display.print(" ");
    display.print(pedalAdjustmentClutchMin);
    display.setCursor(0, 27);
    display.print("  WHEEL:      ");
    display.print(wheelDeg);
    display.setCursor(0, 36);
    display.print("  READ FLAG:  ");
    display.print(readflag ? "ON" : "OFF");
    display.setCursor(0, 45);
    display.print("  INVERTED:   ");
    display.print(modeZeroToMax ? "ON" : "OFF");
    display.setCursor(0, 54);
    display.print("  FLASH MODE: OFF");
    display.display();
}

void displayMenuDataCallibration() {
    display.setCursor(0, 0);
    display.print("  GAS:        ");
    display.print(gasMax);
    display.print(" ");
    display.print(gasMin);
    display.setCursor(0, 9);
    display.print("  BRAKE:      ");
    display.print(brakeMax);
    display.print(" ");
    display.print(brakeMin);
    display.setCursor(0, 18);
    display.print("  CLUTCH:     ");
    display.print(clutchMax);
    display.print(" ");
    display.print(clutchMin);
    display.display();
}

/**
* @brief Увеличить или уменьшить заданное значение с помощью кнопок
*
* Эта функция увеличит или уменьшит заданное значение на 1, если соответствующая кнопка нажата один раз,
* или на 5, если кнопка нажата дважды быстро.
*
* @param value Значение, которое нужно увеличить или уменьшить
* @return Скорректированное значение
*/
int adjustmentFunc(int value) {
  if (button2Down->isSingle())
      value--;
  if (button1Up->isSingle()) 
      value++;            
  if (button2Down->isDouble()) 
      value -= 10;            
  if (button1Up->isDouble()) 
      value += 10;          

  return value;
}

void debugFunc() {
  
}
void saveFunc() {
    EEPROM.put(0, gasMax);
    EEPROM.put(2, brakeMax);
    EEPROM.put(4, clutchMax);
    EEPROM.put(6, gasMin);
    EEPROM.put(8, brakeMin);
    EEPROM.put(10, clutchMin);
    EEPROM.put(12, wheelDeg);
    EEPROM.put(14, modeZeroToMax);
    EEPROM.put(16, readflag);
    EEPROM.put(18, pedalAdjustmentGasMax);
    EEPROM.put(20, pedalAdjustmentGasMin);
    EEPROM.put(22, pedalAdjustmentBrakeMax);
    EEPROM.put(24, pedalAdjustmentBrakeMin);
    EEPROM.put(26, pedalAdjustmentClutchMax);
    EEPROM.put(28, pedalAdjustmentClutchMin);
    resetToMainStatus();
    return;
}

void loadFunc() {
  EEPROM.get(0, gasMax);
  EEPROM.get(2, brakeMax);
  EEPROM.get(4, clutchMax);
  EEPROM.get(6, gasMin);
  EEPROM.get(8, brakeMin);
  EEPROM.get(10, clutchMin);
  EEPROM.get(12, wheelDeg);
  EEPROM.get(14, modeZeroToMax);
  EEPROM.get(16, readflag);
  EEPROM.get(18, pedalAdjustmentGasMax);
  EEPROM.get(20, pedalAdjustmentGasMin);
  EEPROM.get(22, pedalAdjustmentBrakeMax);
  EEPROM.get(24, pedalAdjustmentBrakeMin);
  EEPROM.get(26, pedalAdjustmentClutchMax);
  EEPROM.get(28, pedalAdjustmentClutchMin);
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

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <GyverButton.h>
#include <HID-Project.h>
#include <EEPROM.h>
#include "settings.cpp"

class Core {
private:
    Adafruit_SSD1306* display;
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


    void load();
    void save();
    void read();
    void home()
    void drawCursor();
    void mainFunc();
    void callibrationFunc();
    void resetToMainStatus();
    void drawCursorSelect(int line);
    int adjustmentFunc(int value);
    void handleCursorMovement(int maxIndex);
    void processPedalAdjustment(int& pedalAdjustmentValue);
    void processWheelAdjustment();
    void displayMenuData();
    void displayMenuDataCallibration();
    
public:
    Сore();
    ~Core();
    void loop(int enc);
};
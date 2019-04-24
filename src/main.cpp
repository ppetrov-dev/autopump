#include <Arduino.h>
#include "GyverEncoder.h"
#include <EEPROMex.h>
#include <EEPROMVar.h>
#include "LCD_1602_RUS.h"

#define LCD_BACKL 1         // автоотключение подсветки дисплея (1 - разрешить) 
#define BACKL_TOUT 60       // таймаут отключения дисплея, секунды
#define ENCODER_TYPE 0      // тип энкодера (0 или 1). Если энкодер работает некорректно (пропуск шагов), смените тип
#define ENC_REVERSE 1       // 1 - инвертировать энкодер, 0 - нет
#define DRIVER_VERSION 1    // 0 - маркировка драйвера дисплея кончается на 4АТ, 1 - на 4Т
#define PUPM_AMOUNT 2       // количество помп, подключенных через реле/мосфет
#define START_PIN 4         // подключены начиная с пина
#define SWITCH_LEVEL 0      // реле: 1 - высокого уровня (или мосфет), 0 - низкого
#define PARALLEL 0          // 1 - параллельный полив, 0 - полив в порядке очереди
#define TIMER_START 1       // 1 - отсчёт периода с момента ВЫКЛЮЧЕНИЯ помпы, 0 - с момента ВКЛЮЧЕНИЯ помпы

// названия каналов управления. БУКВУ L НЕ ТРОГАТЬ БЛЕТ!!!!!!
static const wchar_t *relayNames[]  = {
  L"Помпа 1",
  L"Помпа 2"
};

#define CLK 3
#define DT 2
#define SW 0

Encoder enc1(CLK, DT, SW);


// -------- АВТОВЫБОР ОПРЕДЕЛЕНИЯ ДИСПЛЕЯ-------------
// Если кончается на 4Т - это 0х27. Если на 4АТ - 0х3f
#if (DRIVER_VERSION)
LCD_1602_RUS lcd(0x27, 16, 2);
#else
LCD_1602_RUS lcd(0x3f, 16, 2);
#endif
// -------- АВТОВЫБОР ОПРЕДЕЛЕНИЯ ДИСПЛЕЯ-------------

uint32_t pump_timers[PUPM_AMOUNT];
uint32_t pumping_time[PUPM_AMOUNT];
uint32_t period_time[PUPM_AMOUNT];
boolean pump_state[PUPM_AMOUNT];
byte pump_pins[PUPM_AMOUNT];

int8_t current_set;
int8_t current_pump;
boolean now_pumping;

int8_t thisD, thisH, thisM;
long thisPeriod;
boolean startFlag = true;

boolean backlState = true;
uint32_t backlTimer;

// вывести название реле
void drawLabels() {
  lcd.setCursor(1, 0);
  lcd.print("                ");
  lcd.setCursor(1, 0);
  lcd.print(relayNames[current_pump]);
}


// перевод секунд в д ЧЧ:ММ
void s_to_hms(uint32_t period) {
 thisD = floor((long)period / 86400);    // секунды в часы
  thisH = floor((period - (long)thisD*86400) / 3600);
  thisM = floor((period - (long)thisD*86400 - (long)thisH*3600)/60);
}

// перевод д ЧЧ:ММ в секунды
uint32_t hms_to_s() {
return ((long)thisD*86400 + (long)thisH*3600 + (long)thisM*60);
}

// обновляем данные в памяти
void update_EEPROM() {
  EEPROM.updateLong(8 * current_pump, period_time[current_pump]);
  EEPROM.updateLong(8 * current_pump + 4, pumping_time[current_pump]);
}

void backlOn() {
  backlState = true;
  backlTimer = millis();
  lcd.backlight();
}
void backlTick() {
  if (LCD_BACKL && backlState && millis() - backlTimer >= BACKL_TOUT * 1000) {
    backlState = false;
    lcd.noBacklight();
  }
}

void periodTick() {
  for (byte i = 0; i < PUPM_AMOUNT; i++) {            // пробегаем по всем помпам
    if (startFlag ||
        (period_time[i] > 0
         && millis() - pump_timers[i] >= period_time[i] * 1000
         && (pump_state[i] != SWITCH_LEVEL)
         && !(now_pumping * !PARALLEL))) {
      pump_state[i] = SWITCH_LEVEL;
      digitalWrite(pump_pins[i], SWITCH_LEVEL);
      pump_timers[i] = millis();
      now_pumping = true;
      //Serial.println("Pump #" + String(i) + " ON");
    }
  }
  startFlag = false;
}

void flowTick() {
  for (byte i = 0; i < PUPM_AMOUNT; i++) {            // пробегаем по всем помпам
    if (pumping_time[i] > 0
        && millis() - pump_timers[i] >= pumping_time[i] * 1000
        && (pump_state[i] == SWITCH_LEVEL) ) {
      pump_state[i] = !SWITCH_LEVEL;
      digitalWrite(pump_pins[i], !SWITCH_LEVEL);
      if (TIMER_START) pump_timers[i] = millis();
      now_pumping = false;
      //Serial.println("Pump #" + String(i) + " OFF");
    }
  }
}

// отрисовка стрелки и двоеточий
void drawArrow(byte col, byte row) {
  lcd.setCursor(0, 0); lcd.print(" ");
  lcd.setCursor(7, 1); lcd.print(" ");
  lcd.setCursor(10, 1); lcd.print(" ");
  lcd.setCursor(13, 1); lcd.print(":");
  lcd.setCursor(col, row); lcd.write(126);
}

// изменение позиции стрелки и вывод данных
void changeSet() {
  switch (current_set) {
    case 0: drawArrow(0, 0); update_EEPROM();
      break;
    case 1: drawArrow(7, 1);
      break;
    case 2: drawArrow(10, 1);
      break;
    case 3: drawArrow(13, 1);
      break;
    case 4: drawArrow(7, 1);
      break;
    case 5: drawArrow(10, 1);
      break;
    case 6: drawArrow(13, 1);
      break;
  }
  lcd.setCursor(0, 1);
  if (current_set < 4) {
    lcd.print(L"ПАУЗА ");
    s_to_hms(period_time[current_pump]);
  }
  else {
    lcd.print(L"РАБОТА");
    s_to_hms(pumping_time[current_pump]);
  }
  lcd.setCursor(8, 1);
  lcd.print(thisD);
  lcd.print("д");
  lcd.setCursor(11, 1);
  if (thisH < 10) lcd.print(0);
  lcd.print(thisH);
  lcd.setCursor(14, 1);
  if (thisM < 10) lcd.print(0);
  lcd.print(thisM);
}
// тут меняем номер помпы и настройки
void changeSettings(int increment) {
  if (current_set == 0) {
    current_pump += increment;
    if (current_pump > PUPM_AMOUNT - 1) current_pump = PUPM_AMOUNT - 1;
    if (current_pump < 0) current_pump = 0;
    s_to_hms(period_time[current_pump]);
    drawLabels();
  } else {
    if (current_set == 1 || current_set == 4) {
      thisD += increment;
    } else if (current_set == 2 || current_set == 5) {
      thisH += increment;
    } else if (current_set == 3 || current_set == 6) {
      thisM += increment;
    }
     if (thisD > 9) {
      thisD = 9;
    }
    if (thisM > 59) {
      thisM = 0;
    }
    if (thisH > 59) {
      thisH = 0;
    }
    if (thisM < 0) {
       thisM = 59;
    }
    if (thisH < 0) {
      thisH = 59;
    }
    if (thisD < 0) {
      thisD = 9;
    }
    if (current_set < 4) period_time[current_pump] = hms_to_s();
    else pumping_time[current_pump] = hms_to_s();
  }
}
void encoderTick() {
  enc1.tick();    // отработка энкодера

  if (enc1.isTurn()) {                               // если был совершён поворот
    if (backlState) {
      backlTimer = millis();      // сбросить таймаут дисплея
      if (enc1.isRight()) {
        if (++current_set >= 7) current_set = 6;
      } else if (enc1.isLeft()) {
        if (--current_set < 0) current_set = 0;
      }

      if (enc1.isRightH())
        changeSettings(1);
      else if (enc1.isLeftH())
        changeSettings(-1);

      changeSet();
    } else {
      backlOn();      // включить дисплей
    }
  }
}

void setup() {
  // --------------------- КОНФИГУРИРУЕМ ПИНЫ ---------------------
  for (byte i = 0; i < PUPM_AMOUNT; i++) {            // пробегаем по всем помпам
    pump_pins[i] = START_PIN + i;                     // настраиваем массив пинов
    pinMode(START_PIN + i, OUTPUT);                   // настраиваем пины
    digitalWrite(START_PIN + i, !SWITCH_LEVEL);       // выключаем от греха
  }
  // --------------------- ИНИЦИАЛИЗИРУЕМ ЖЕЛЕЗО ---------------------
  Serial.begin(9600);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  //enc1.setStepNorm(1);
  //attachInterrupt(0, encISR, CHANGE);
  enc1.setType(ENCODER_TYPE);
  if (ENC_REVERSE) enc1.setDirection(REVERSE);

  // --------------------- СБРОС НАСТРОЕК ---------------------
  if (!digitalRead(SW)) {          // если нажат энкодер, сбросить настройки до 1
    lcd.setCursor(0, 0);
    lcd.print("Сброс настроек");
    for (byte i = 0; i < 500; i++) {
      EEPROM.writeLong(i, 0);
    }
  }
  while (!digitalRead(SW));        // ждём отпускания кнопки
  lcd.clear();                     // очищаем дисплей, продолжаем работу

  // --------------------------- НАСТРОЙКИ ---------------------------
  // в ячейке 1023 должен быть записан флажок, если его нет - делаем (ПЕРВЫЙ ЗАПУСК)
  if (EEPROM.read(1023) != 5) {
    EEPROM.writeByte(1023, 5);

    // для порядку сделаем 1 ячейки с 0 по 500
    for (byte i = 0; i < 500; i += 4) {
      EEPROM.writeLong(i, 0);
    }
  }

  for (byte i = 0; i < PUPM_AMOUNT; i++) {            // пробегаем по всем помпам
    period_time[i] = EEPROM.readLong(8 * i);          // читаем данные из памяти. На чётных - период (ч)
    pumping_time[i] = EEPROM.readLong(8 * i + 4);     // на нечётных - полив (с)

    if (SWITCH_LEVEL)			// вырубить все помпы
      pump_state[i] = 0;
    else
      pump_state[i] = 1;
  }

  // ---------------------- ВЫВОД НА ДИСПЛЕЙ ------------------------
  drawLabels();
  changeSet();
}

void loop() {
  encoderTick();
  periodTick();
  flowTick();
  backlTick();
}
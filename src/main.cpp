#include "OneButton.h"
#include "MyRotaryEncoder.h"
#include "MyTimer.h"
#include "AutoWateringLcd/AutoWateringLcd.h"
#include "AutoWateringStateMachine/AutoWateringStateMachine.h"
#include "Pump/Pump.h"
#include "settings.h"
#include "enums.h"
#include "MyDateTimeConverters.h"
#include "DataStorage/DataStorage.h"
#include "RTC/RealTimeClock.h"

AutoWateringStateMachine _autoWateringStateMachine;
MyRotaryEncoder _autoWateringEncoder = MyRotaryEncoder(PIN_EncoderClk, PIN_EncoderDt, PIN_EncoderSw);

AutoWateringLcd _autoWateringLcd = AutoWateringLcd(16, 2);
MyTimer _timer;
RealTimeClock _realTimeClock;

DataStorage _dataStorage(PUMP_AMOUNT);
Pump *_pumps[PUMP_AMOUNT];

OneButton _pumpButton1 = OneButton(PIN_Button1, true, true);
OneButton _pumpButton2 = OneButton(PIN_Button2, true, true);
OneButton *_buttons[PUMP_AMOUNT] = {&_pumpButton1, &_pumpButton2};

bool _isWatering;

Pump *GetSelectedPump()
{
  auto pumpIndex = _autoWateringLcd.GetSelectedPumpIndex();
  auto pump = _pumps[pumpIndex];
  return pump;
}

void UpdateSelectedValuesToSelectedPump(unsigned long waitTimeInMinutes, unsigned long workTimeInSeconds)
{
  auto pumpIndex = _autoWateringLcd.GetSelectedPumpIndex();
  auto pump = _pumps[pumpIndex];

  pump->WaitTimeInMinutes = waitTimeInMinutes;
  pump->WorkTimeInSeconds = workTimeInSeconds;
}

void UpdateSelectedValuesFromSelectedPump()
{
  auto pump = GetSelectedPump();
  auto waitTimeInMinutes = pump->WaitTimeInMinutes;
  auto waitTimeInSeconds = MyDateTimeConverters::MinutesToSeconds(waitTimeInMinutes);
  auto workTimeInSeconds = pump->WorkTimeInSeconds;

  _autoWateringLcd.UpdateWaitTimeFromSeconds(waitTimeInSeconds);
  _autoWateringLcd.UpdateWorkTimeFromSeconds(workTimeInSeconds);
}

void UpdateSelectedPumpWaitAndWorkTime()
{
  auto pump = GetSelectedPump();
  auto workTimeInSeconds = _autoWateringLcd.ConvertWorkTimeToSeconds();
  auto waitTimeInSeconds = _autoWateringLcd.ConvertWaitTimeToSeconds();

  pump->WorkTimeInSeconds = workTimeInSeconds;
  pump->WaitTimeInMinutes = MyDateTimeConverters::SecondsToMinutes(waitTimeInSeconds);
}

void TryPrintSelectedPumpStatus()
{
  switch (_autoWateringStateMachine.GetState())
  {
  case SelectPumpState:
  case SelectSettingsState:
    auto pump = GetSelectedPump();
    auto status = pump->GetStatus();
    _autoWateringLcd.PrintOnRow(1, status);
    break;
  }
}
#pragma region Pump Handlers
void OnPumpStopped(Pump* pump){
  int pumpIndex = 0;
  while (_pumps[pumpIndex]!= pump && pumpIndex  <= PUMP_AMOUNT)
    pumpIndex++;
  if(pumpIndex == PUMP_AMOUNT)
    return;

  Serial.println("OnPumpStopped| pumpIndex: " + String(pumpIndex));
  _dataStorage.SaveDataIfNeeded(pumpIndex, _realTimeClock.GetNowTimeStamp());
}
#pragma endregion
#pragma region AutoWateringStateMachine Handlers
void OnStateChanged()
{
  auto currentState = _autoWateringStateMachine.GetState();
  _autoWateringLcd.UpdateState(currentState);
  TryPrintSelectedPumpStatus();
}
void OnStateMachineLeftSettings()
{
  auto waitTimeInSeconds = _autoWateringLcd.ConvertWaitTimeToSeconds();
  auto waitTimeInMinutes = MyDateTimeConverters::SecondsToMinutes(waitTimeInSeconds);
  auto workTimeInSeconds = _autoWateringLcd.ConvertWorkTimeToSeconds();
  UpdateSelectedValuesToSelectedPump(waitTimeInMinutes, workTimeInSeconds);
  _dataStorage.SaveDataIfNeeded(_autoWateringLcd.GetSelectedPumpIndex(), waitTimeInMinutes, workTimeInSeconds);
}
#pragma endregion

#pragma region Buttons Handlers
void OnButtonLongPressStart(int index)
{
  _pumps[index]->ForceStart(ForcedlyStarted);
}
void OnButtonDoubleClick(int index)
{
  auto pump = _pumps[index];
  if (pump->GetIsWorking())
  {
    pump->Stop();
    return;
  }
  _pumps[index]->ForceStart(ForcedlyStartedWithTimer);
}
void OnButtonLongPressStop(int index)
{
  _pumps[index]->Stop();
}

#pragma endregion
void setup()
{
  if (DEBUG)
  {
    Serial.begin(9600);
    Serial.println("Serial OK");
  }
  Wire.begin();
  _realTimeClock.Begin();
  Serial.print("Current DateTime: ");
  Serial.println(_realTimeClock.GetStringNow());

  _autoWateringLcd.IsAutoOff = IS_LCD_AUTO_OFF;
  _autoWateringLcd.TimeoutInSeconds = Lcd_TIMEOUT_SECONDS;
  _autoWateringLcd.Init(PUMP_AMOUNT);
  _autoWateringLcd.AttachOnSelectedPumpChanged([]() { TryPrintSelectedPumpStatus(); });
  _autoWateringLcd.Refresh(_autoWateringStateMachine.GetState());

  _dataStorage.Init();
  auto nowTimeStampInSeconds = _realTimeClock.GetNowTimeStamp();
  for (int i = 0; i < PUMP_AMOUNT; i++)
  {
    auto pump = new Pump(PIN_FirstPump + i);
    _pumps[i] = pump;
    pump->Init(FORCEDLY_STARTED_PUMP_SECONDS, RELAY_TYPE);
    auto isDataStorageReady = _dataStorage.GetIsReady(i);
    if (isDataStorageReady)
    {
      auto data = _dataStorage.GetData(i);
      pump->WaitTimeInMinutes = data->WaitTimeInMinutes;
      pump->WorkTimeInSeconds = data->WorkTimeInSeconds;
      auto timeStampInSeconds = data->LastWateringTimeStampInSeconds;

      auto timeOffsetInSeconds = nowTimeStampInSeconds - timeStampInSeconds;
      if (timeOffsetInSeconds >= ACCEPTABLE_TIME_OFFSET_SECONDS)
        pump->ResetOffsetTime(timeOffsetInSeconds);
    }
    else
      _dataStorage.SaveDataIfNeeded(i, pump->WaitTimeInMinutes, pump->WorkTimeInSeconds, nowTimeStampInSeconds);
      pump->AttachOnStopped(&OnPumpStopped);
  }

  _autoWateringStateMachine.AttachOnIncreaseValue([]() { _autoWateringLcd.UpdateSelectedValues(_autoWateringStateMachine.GetState(), 1); });
  _autoWateringStateMachine.AttachOnDecreaseValue([]() { _autoWateringLcd.UpdateSelectedValues(_autoWateringStateMachine.GetState(), -1); });
  _autoWateringStateMachine.AttachOnStateChanged(&OnStateChanged);
  _autoWateringStateMachine.AttachOnLeftSettings(&OnStateMachineLeftSettings);
  _autoWateringStateMachine.AttachOnBeforeEnterToSettings([]() { UpdateSelectedValuesFromSelectedPump(); });

  pinMode(PIN_Button1, INPUT_PULLUP);
  pinMode(PIN_Button2, INPUT_PULLUP);
  _pumpButton1.attachLongPressStart([]() { OnButtonLongPressStart(0); });
  _pumpButton1.attachLongPressStop([]() { OnButtonLongPressStop(0); });
  _pumpButton1.attachDoubleClick([]() { OnButtonDoubleClick(0); });
  _pumpButton2.attachLongPressStart([]() { OnButtonLongPressStart(1); });
  _pumpButton2.attachLongPressStop([]() { OnButtonLongPressStop(1); });
  _pumpButton2.attachDoubleClick([]() { OnButtonDoubleClick(1); });

  _autoWateringEncoder.SetEncoderType(ENCODER_TYPE);
  _autoWateringEncoder.SetEncoderDirection(IS_ENCODER_REVERSED);

  _autoWateringEncoder.AttachOnLeftTurn([]() { _autoWateringStateMachine.Run(EncoderLeftTurnCommand); });
  _autoWateringEncoder.AttachOnRightTurn([]() { _autoWateringStateMachine.Run(EncoderRightTurnCommand); });
  _autoWateringEncoder.AttachOnLeftHoldTurn([]() { _autoWateringStateMachine.Run(EncoderHoldLeftTurnCommand); });
  _autoWateringEncoder.AttachOnRightHoldTurn([]() { _autoWateringStateMachine.Run(EncoderHoldRightTurnCommand); });
  _autoWateringEncoder.AttachOnClick([]() { _autoWateringStateMachine.Run(EncoderClickCommand); });

  _timer.SetInterval(1000);
  _timer.AttachOnTick(&TryPrintSelectedPumpStatus);
  _timer.Start();
}

void HandleButtonsTick()
{
  for (int i = 0; i < PUMP_AMOUNT; i++)
  {
    auto button = _buttons[i];
    button->tick();
  }
}

void UpdateIsWatering()
{
  for (int i = 0; i < PUMP_AMOUNT; i++)
  {
    auto pump = _pumps[i];
    _isWatering = pump->GetIsWorking();
    if (_isWatering)
      break;
  }
}

void HandlePumpsTick()
{
  for (int i = 0; i < PUMP_AMOUNT; i++)
  {
    auto pump = _pumps[i];
    auto beforeIsPumpWatering = pump->GetIsWorking();
    if (_isWatering && IS_PARALLEL_WATERING_DISABLED && !beforeIsPumpWatering)
      continue;

    pump->Tick();

    if (pump->GetIsWorking() && IS_PARALLEL_WATERING_DISABLED)
      break;
  }
}
void loop()
{
  _autoWateringEncoder.Tick();
  _autoWateringLcd.Tick();
  _timer.Tick();

  HandleButtonsTick();
  UpdateIsWatering();
  HandlePumpsTick();
}
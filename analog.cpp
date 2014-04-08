//----------------------------------------------------------------------------

//Модуль управления оборудованием

//----------------------- Используемые ресурсы: ------------------------------

//Класс TAnalog является интерфейсом для аналоговой части схемы.
//Он производит преобразование кода ЦАП и АЦП в значение и обратно
//согласно текущей калибровке. Выполняет процесс фильтрации данных АЦП.
//Осуществляет управление включением выхода источника OUT ON/OFF.
//Выполняет процесс измерения температуры с помощью модуля Therm и
//управляет вентилятором.

//----------------------------------------------------------------------------

#include "main.h"
#include "analog.h"
#include "display.h"
#include "sound.h"

//----------------------------------------------------------------------------
//----------------------------- Класс TScaler: -------------------------------
//----------------------------------------------------------------------------

#define SCALE ((1LL << (64 - 16 - 1)) / VMAX) //масштаб коэффициентов

//---------- Вычисление калибровочных коэффициентов по двум точкам: ----------

bool TScaler::Calibrate(uint16_t p1, uint16_t c1,
                        uint16_t p2, uint16_t c2)
{
  //TODO: проверка пригодности значений для калибровки
  //if((p2 <= p1) || (c2 <= c1)) return(0);
  uint16_t dp = p2 - p1;
  //if(dp < MIN_DP) return(0);
  uint16_t dc = c2 - c1;
  //if(dc < MIN_DC) return(0);
  Kx = (SCALE * dp + dc / 2) / dc;
  Sx = Kx * c1 - SCALE * p1;
  return(1);
}

//------------------- Преобразование кода в величину: ------------------------

uint16_t TScaler::CodeToValue(uint16_t code)
{
  int32_t v = (Kx * code - Sx + SCALE / 2) / SCALE;
  //TODO: если нужно индицировать ток DP, снять это ограничение:
  if(v < 0) v = 0;
  if(v > VMAX) v = VMAX;
  return(v);
}

//------------------- Преобразование величины в код: -------------------------

uint16_t TScaler::ValueToCode(uint16_t value)
{
  int32_t c = (SCALE * value + Sx + Kx / 2) / Kx;
  if(c < 0) c = 0;
  if(c > DAC_MAX_CODE) c = DAC_MAX_CODE;
  return(c);
}

//----------------------------------------------------------------------------
//---------------------------- Класс TAnalog: -------------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TAnalog::TAnalog(void)
{
  Therm = new TTherm();
  Fan = new TFan();
  Pin_CC.Init(IN_PULL, PULL_DN);
  Pin_ON.Init(OUT_PP_2M, OUT_LO);
  AdcV = new TAdc<ADC_CH_V, ADC_PIN_V>();
  AdcI = new TAdc<ADC_CH_I, ADC_PIN_I>();
  DacV = new TDac<DAC_CH_V>();
  DacI = new TDac<DAC_CH_I>();
  
  CCTimer = new TSoftTimer(CVCC_DEL);
  CCTimer->Force();
  CVTimer = new TSoftTimer(CVCC_DEL);
  CVTimer->Force();

  OvpTimer = new TSoftTimer();
  OcpTimer = new TSoftTimer();
  OppTimer = new TSoftTimer();
  ProtSt = PR_OK;

  CvCcSt = PS_UNREG;
  PinCCState = Pin_CC;
  Out = 0;
  
  CalibData = new TParamList(CAL_CNT);
  //                            Type       Name   Min   Nom   Max
  CalibData->AddItem(new TParam(PT_V,     " P1 ", VP1L, VPD1, VP1H)); //CAL_VP1
  CalibData->AddItem(new TParam(PT_VC,    " C1 ",    1, VCD1, DACM)); //CAL_VC1
  CalibData->AddItem(new TParam(PT_V,     " P2 ", VP2L, VPD2, VP2H)); //CAL_VP2
  CalibData->AddItem(new TParam(PT_VC,    " C2 ",    1, VCD2, DACM)); //CAL_VC2
  CalibData->AddItem(new TParam(PT_I,     " P1 ", IP1L, IPD1, IP1H)); //CAL_IP1
  CalibData->AddItem(new TParam(PT_IC,    " C1 ",    1, ICD1, DACM)); //CAL_IC1
  CalibData->AddItem(new TParam(PT_I,     " P2 ", IP2L, IPD2, IP2H)); //CAL_IP2
  CalibData->AddItem(new TParam(PT_IC,    " C2 ",    1, ICD2, DACM)); //CAL_IC2
  CalibData->AddItem(new TParam(PT_NYDEF, "Stor",    0,    1,    2)); //CAL_STR
  CalibData->AddItem(new TParam(PT_VC,        "",    1, VMD1, ADCM)); //CAL_VM1
  CalibData->AddItem(new TParam(PT_VC,        "",    1, VMD2, ADCM)); //CAL_VM2
  CalibData->AddItem(new TParam(PT_IC,        "",    1, IMD1, ADCM)); //CAL_IM1
  CalibData->AddItem(new TParam(PT_IC,        "",    1, IMD2, ADCM)); //CAL_IM2
  CalibData->EeSection = new TCrcSection(CalibData->ItemsCount);
  CalibData->ReadFromEeprom();

  CalibAll();
}

//---------- Коррекция пределов параметров согласно MAX_V и MAX_I: -----------

void TAnalog::TrimParamsLimits(void)
{
  CalibData->Items[CAL_VP2]->Max = Data->TopData->Items[PAR_MAXV]->Value;
  //коррекция кода, если пришлось поменять точку:
  if(CalibData->Items[CAL_VP2]->Validate())
    CalibData->Items[CAL_VC2]->Value =
      DacV->ValueToCode(CalibData->Items[CAL_VP2]->Value);

  CalibData->Items[CAL_IP2]->Max = Data->TopData->Items[PAR_MAXI]->Value;
  //коррекция кода, если пришлось поменять точку:
  if(CalibData->Items[CAL_IP2]->Validate())
    CalibData->Items[CAL_IC2]->Value =
      DacV->ValueToCode(CalibData->Items[CAL_IP2]->Value);
}

//-------------------------- Процесс измерения: ------------------------------

void TAnalog::Execute(void)
{
  Therm->Execute();
  AdcV->Execute();
  AdcI->Execute();
  CvCcControl();
  ThermalControl();
  Protection();
  Supervisor();
}

//----------------- Загрузка калибровки для всего сразу: ---------------------

//TODO: все функции калибровки должны возвращать флаг ошибки

void TAnalog::CalibAll(void)
{
  CalibDacV();
  CalibDacI();
  CalibAdcV();
  CalibAdcI();
}

//-------------------- Загрузка калибровки для DAC_V: ------------------------

void TAnalog::CalibDacV(void)
{
  DacV->Calibrate(CalibData->Items[CAL_VP1]->Value,
                  CalibData->Items[CAL_VC1]->Value,
                  CalibData->Items[CAL_VP2]->Value,
                  CalibData->Items[CAL_VC2]->Value);
}
  
//-------------------- Загрузка калибровки для DAC_I: ------------------------

void TAnalog::CalibDacI(void)
{
  DacI->Calibrate(CalibData->Items[CAL_IP1]->Value,
                  CalibData->Items[CAL_IC1]->Value,
                  CalibData->Items[CAL_IP2]->Value,
                  CalibData->Items[CAL_IC2]->Value);
}

//-------------------- Загрузка калибровки для ADC_V: ------------------------

//Если параметр равен CAL_VM1, читается первая точка кода ADC_V.
//Если параметр равен CAL_VM2, читается вторая точка кода ADC_V.
//Иначе используются прежние значения параметров.

void TAnalog::CalibAdcV(char p)
{
  if(p == CAL_VM1) CalibData->Items[CAL_VM1]->Value = AdcV->Code;
  if(p == CAL_VM2) CalibData->Items[CAL_VM2]->Value = AdcV->Code;
  AdcV->Calibrate(CalibData->Items[CAL_VP1]->Value,
                  CalibData->Items[CAL_VM1]->Value,
                  CalibData->Items[CAL_VP2]->Value,
                  CalibData->Items[CAL_VM2]->Value);
}
  
//-------------------- Загрузка калибровки для ADC_I: ------------------------

//Если параметр равен CAL_IM1, читается первая точка кода ADC_I.
//Если параметр равен CAL_IM2, читается вторая точка кода ADC_I.
//Иначе используются прежние значения параметров.

void TAnalog::CalibAdcI(char p)
{
  if(p == CAL_IM1) CalibData->Items[CAL_IM1]->Value = AdcI->Code;
  if(p == CAL_IM2) CalibData->Items[CAL_IM2]->Value = AdcI->Code;
  AdcI->Calibrate(CalibData->Items[CAL_IP1]->Value,
                  CalibData->Items[CAL_IM1]->Value,
                  CalibData->Items[CAL_IP2]->Value,
                  CalibData->Items[CAL_IM2]->Value);
}

//------------------------ Обслуживание защиты: ------------------------------

inline void TAnalog::Protection(void)
{
  //OVP:
  if(AdcV->FastReady())
  {
    uint16_t vp = Data->ProtData->Items[PAR_OVP]->Value;
    //если OVP = MAX_V, защита выключена:
    if((vp < Data->ProtData->Items[PAR_OVP]->Max) && Out)
    {
      if(Analog->AdcV->FastValue >= vp)
      {
        if(OvpTimer->Over())
        {
          OutControl(0);
          ProtSt |= PR_OVP;
        }
      }
      else
      {
        OvpTimer->Start(Data->SetupData->Items[PAR_DEL]->Value);
      }
    }
  }
  //OCP:
  if(AdcI->FastReady())
  {
    uint16_t ip = Data->ProtData->Items[PAR_OCP]->Value;
    //если OCP = MAX_I, защита выключена:
    if((ip < Data->ProtData->Items[PAR_OCP]->Max) && Out)
    {
      if(Analog->AdcI->FastValue >= ip)
      {
        if(OcpTimer->Over())
        {
          OutControl(0);
          ProtSt |= PR_OCP;
        }
      }
      else
      {
        OcpTimer->Start(Data->SetupData->Items[PAR_DEL]->Value);
      }
    }
   //OPP:
    uint16_t pp = Data->ProtData->Items[PAR_OPP]->Value;
    //если OPP = MAX_P, защита выключена:
    if((pp < Data->ProtData->Items[PAR_OPP]->Max) && Out)
    {
      uint16_t pow = (uint32_t)Analog->AdcI->FastValue *
                               Analog->AdcV->FastValue / VI2P;
      if(pow >= pp)
      {
        if(OppTimer->Over())
        {
          OutControl(0);
          ProtSt |= PR_OPP;
        }
      }
      else
      {
        OppTimer->Start(Data->SetupData->Items[PAR_DEL]->Value);
      }
    }
  }
}

//------------------------- Супервизор питания: ------------------------------
  
inline void TAnalog::Supervisor(void)
{
  if(PWR->CSR & PWR_CSR_PVDO)
  {
    Pin_ON = 0;
    Display->Disable();
    TSysTimer::Delay_ms(1000);
    __disable_interrupt();
    NVIC_SystemReset();
  }
}

//---------------------- Управление вентилятором: ----------------------------

inline void TAnalog::ThermalControl(void)
{
  if(Therm->Update())
  {
    Temp = Therm->Value;
    TempUpdate = 1;
    //ошибка термометра:
    if(Temp == TEMP_FAIL)
    {
      ProtSt &= ~PR_OTP;
      Fan->SetSpeed(0);
    }
    else
    {
      //PI-регулятор вентилятора:
      Fan->PiReg(Data->SetupData->Items[PAR_FAN]->Value, Temp);
      //проверка перегрева:
      if(Temp > Data->SetupData->Items[PAR_ALA]->Value + T_OVER)
      {
        ProtSt |= PR_OTP;
        OutControl(0);
        Fan->SetSpeed(FAN_MAX_SPEED);
      }
      //звуковая индикация опасного нагрева:
      else if(Temp > Data->SetupData->Items[PAR_ALA]->Value)
      {
        ProtSt &= ~PR_OTP;
        Sound->Alarm(); //звук опасного нагрева
        Fan->SetSpeed(FAN_MAX_SPEED);
      }
      else
      {
        ProtSt &= ~PR_OTP;
      }
    }
  }
}

//----------------- Чтение флага обновления температуры: ---------------------

bool TAnalog::UpdTemp(void)
{
  if(TempUpdate)
  {
    TempUpdate = 0;
    return(1);
  }
  return(0);
}

//---------- Принудительная установка флага обновления температуры: ----------

void TAnalog::ForceTemp(void)
{
  TempUpdate = 1;
}

//-------------------------- Чтение температуры: -----------------------------

int16_t TAnalog::GetTemp(void)
{
  return(Temp);
}

//---------------------- Чтение скорости вентилятора: ------------------------

char TAnalog::GetSpeed(void)
{
  return(Fan->GetSpeed());
}

//----------------------- Чтение состояния защиты: ---------------------------

char TAnalog::GetProtSt(void)
{
  return(ProtSt);
}

//------------------------- Сброс OVP/OCP/OPP: -------------------------------

void TAnalog::ClrProtSt(void)
{
  ProtSt &= ~(PR_OVP | PR_OCP | PR_OPP);
}

//-------------------- Обслуживание режимов CV/CC: ---------------------------

inline void TAnalog::CvCcControl(void)
{
  if(TSysTimer::Tick)
  {
    char state = PS_UNREG;
    if(Out)
    {
      bool pinCC = Pin_CC;
      if(pinCC != PinCCState)
      {
        if(pinCC) CCTimer->Start();
          else CVTimer->Start();
        PinCCState = pinCC;  
      }
      if( pinCC || !CCTimer->Over()) state |= PS_CC;
      if(!pinCC || !CVTimer->Over()) state |= PS_CV;
      if((CvCcSt & PS_CC) != (state & PS_CC))
      {
        if(state & PS_CC) Sound->Alarm();
          else Sound->Alert();
      }
    }
    CvCcSt = state;
    Display->LedCC = CvCcSt & PS_CC;
    Display->LedCV = CvCcSt & PS_CV;
  }
}

//------------------------ Чтение режимов CV/CC: -----------------------------

char TAnalog::GetCvCcSt(void)
{
  return(CvCcSt);
}

//-------------------- Включение/выключение выхода: --------------------------

void TAnalog::OutControl(bool on)
{
  Out = on;
  Data->OutOn = on;
  //обнуление ЦАП:
  DacV->OnOff(on);
  DacI->OnOff(on);
  //если включен Down Programmer, то Pin_ON не выключаем: 
  if(Data->SetupData->Items[PAR_DNP]->Value)
    Pin_ON = 1;
      else Pin_ON = on;
  if(!Out)
  {
    CvCcSt = PS_UNREG;
    Display->LedOut = 0;
    Display->LedCC = 0;
    Display->LedCV = 0;
  }
  else
  {
    CvCcSt = PS_CV;
    OvpTimer->Start(Data->SetupData->Items[PAR_DEL]->Value);
    OcpTimer->Start(Data->SetupData->Items[PAR_DEL]->Value);
    Display->LedOut = 1;
  }
}

//---------------------- Чтение состояния выхода: ----------------------------

bool TAnalog::OutState(void)
{
  return(Out);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------

//Модуль реализации меню

//----------------------------------------------------------------------------

#include "main.h"
#include "menu.h"
#include "sound.h"
#include "encoder.h"
#include "eeprom.h"
#include "control.h"
#include "analog.h"

//------------------------------- Константы: ---------------------------------

#define TIMEOUT_SPLASH  2000 //время индикации splash, мс
#define TIMEOUT_MAIN    5000 //задержка возврата из установки V и I, мс
#define TIMEOUT_SETUP  10000 //задержка возврата из меню, мс
#define TIMEOUT_LOCK    1000 //время индикации Lock

//----------------------------------------------------------------------------
//--------------------- Абстрактный класс TMenuItem: -------------------------
//----------------------------------------------------------------------------

//------------------------- Вывод имени параметра: ---------------------------

void TMenuItem::ShowName(void)
{
  Par->ShowName();
}

//----------------------- Вывод значения параметра: --------------------------

void TMenuItem::ShowValue(void)
{
  Par->ShowValue();
}

//------------------------- Загрузка параметра: ------------------------------

void TMenuItem::LoadParam(TParam* p)
{
  Par = p;
  //параметры, которые не должны сохраняться,
  //инициализируются номинальными значениями:
  if(!Par->Savable())
    Par->Value = Par->Nom;
  Display->Blink(BLINK_NO);
  Par->ShowName();
  Par->ShowValue();
}

//------------------------- Вход в редактирование: ---------------------------

void TMenuItem::EditEnter(void)
{
  Edit = 1;
  Par->ShowValue();
  if(Par->Type == PT_V)
    Display->Blink(BLINK_V);
      else Display->Blink(BLINK_I);
  BackupV = Par->Value;
}

//------------------------ Выход из редактирования: --------------------------

void TMenuItem::EditExit(void)
{
  if(Edit)
  {
    Edit = 0;
    Display->Blink(BLINK_NO);
    Sound->High();
    Params->SaveToEeprom(ParIndex);
  }
}

//----------------- Выход из редактирования без сохранения: ------------------

void TMenuItem::EditEscape(void)
{
  if(Edit)
  {
    Edit = 0;
    Par->Value = BackupV;
    Display->Blink(BLINK_NO);
  }
  Sound->High();
}

//---------------------------- Событие таймера: ------------------------------

void TMenuItem::OnTimer(void)
{
  EditEscape();
  MnuIndex = MNU_MAIN;
}

//----------------------------------------------------------------------------
//---------------------------- Класс TMenuInfo: ------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuInfo::Init(void)
{
  static const char InfoStr[INFOS][2 * DIGS + 1] =
  { {"PSL-3604"}, {"Err- EEP"} };

  Edit = 0;
  Display->SetPos(0, 0);
  Display->PutString(&InfoStr[ParIndex][0]);
  if(ParIndex == INF_SPLASH)
    Timeout = TIMEOUT_SPLASH;
      else Timeout = 0; //не выходить по таймеру из меню ошибок
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuInfo::OnKeyboard(KeyMsg_t &msg)
{
  MnuIndex = MNU_MAIN;
  if(msg != KBD_OUT) msg = KBD_NOP;
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuInfo::OnEncoder(int8_t &step)
{
  Sound->Beep();
  MnuIndex = MNU_MAIN;
  step = ENC_NOP;
}

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuMain: -------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuMain::Init(void)
{
  ParIndex = 0; //внешний индекс параметра здесь не используется
  Edit = 0;
  Power = 0;
  Data->SetVI();
  Analog->AdcI->Force();
  Analog->AdcV->Force();
  Params->Items[PAR_V]->ShowValue();
  Params->Items[PAR_I]->ShowValue();
  Display->Blink(BLINK_NO);
  Timeout = TIMEOUT_MAIN;
}

//---------------------------- Обновление меню: ------------------------------

//TODO: сделать корректную индикацию для втекающего тока
//при включенном и выключенном DP.

void TMenuMain::Execute(void)
{
  //индикация P:
  if(!Edit && Power)
  {
    if(Analog->AdcV->Ready() && Analog->AdcI->Ready())
    {
      Display->SetPos(0, 0);
      Display->PutString(" P- ");
      Display->SetPos(1, 0);
      uint32_t p = (uint32_t)Analog->AdcV->Value * Analog->AdcI->Value / 100;
      Display->PutIntF(p, 4, 3 + AUTO_SCALE);
    }
    return;
  }
  //индикация V:
  //если новое значение АЦП готово и V не редактируется,
  //то требуется отображать значение V
  if(Analog->AdcV->Ready() && !(Edit && ParIndex == PAR_V))
  {
    //если выбран режим отображения измеренного значения
    if((Data->SetupData->Items[PAR_GET]->Value == ON) ||
       //или если выход включен
       (Analog->OutState() &&
       //и не находимся в CV c выбранным режимом отображения
       //установленной величины, 
       !((Data->SetupData->Items[PAR_SET]->Value == ON) &&
       (Analog->GetCvCcSt() & PS_CV))))
    {
      //то отображается измеренное значение:
      Display->SetPos(0, 0);
      Display->PutIntF(Analog->AdcV->Value, 4, 2);
    }
    else
    {
      //иначе отображается установленное значение:
      Params->Items[PAR_V]->ShowValue();
    }
  }
  //индикация I:
  //если новое значение АЦП готово и I не редактируется,
  //то требуется отображать значение I
  if(Analog->AdcI->Ready() && !(Edit && ParIndex == PAR_I))
  {
    //если выбран режим отображения измеренного значения
    if((Data->SetupData->Items[PAR_GET]->Value == ON) ||
       //или если выход включен
       (Analog->OutState() && 
       //и не находимся в CC c выбранным режимом отображения
       //установленной величины, 
       !((Data->SetupData->Items[PAR_SET]->Value == ON) &&
       (Analog->GetCvCcSt() & PS_CC))))
    {
      //то отображается измеренное значение:
      Display->SetPos(1, 0);
      //TODO: если нужно индицировать ток DP:
      //Display->PutIntF(Analog->AdcI->Value, 4, 3 + AUTO_SCALE);
      Display->PutIntF(Analog->AdcI->Value, 4, 3);
    }
    else
    {
      //иначе если включен DP, выход выключен, но есть напряжение,
      if((Data->SetupData->Items[PAR_DNP]->Value == ON) &&
         !Analog->OutState() && Analog->AdcV->Value >= DNP_VC)
      {
        //отображаются символы "dnP":
        Display->SetPos(1, 0);
        Display->PutString(" dnP");
      }
      //иначе если выбран режим предпросмотра установленного значения I
      //или выход включен,
      else if((Data->SetupData->Items[PAR_PRC]->Value == ON) ||
              Analog->OutState())
      {
        //отображается установленное значение:
        Params->Items[PAR_I]->ShowValue();
      }
      else
      {
        //иначе отображается нулевое значение I:
        Display->SetPos(1, 0);
        Display->PutIntF(0, 4, 3);
      }
    }
  }
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuMain::OnKeyboard(KeyMsg_t &msg)
{
  if(Data->SetupData->Items[PAR_LOCK]->Value == ON)
  {
    if((msg == KBD_SETV) ||
       (msg == KBD_SETI) ||
       (msg == KBD_FINE) ||
       (msg == KBD_ENC))
    {
      ParIndex = 0;
      MnuIndex = MNU_SETUP; //переход в меню MNU_SETUP
      msg = KBD_NOP;
      return;
    }
  }
  if(msg == KBD_SETV)
  {
    if(Edit && ParIndex == PAR_V)
    {
      EditExit();
    }
    else
    {
      EditExit();
      ParIndex = PAR_V;
      Par = Params->Items[ParIndex];
      EditEnter();
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETI)
  {
    if(Edit && ParIndex == PAR_I)
    {
      EditExit();
    }
    else
    {
      EditExit();
      ParIndex = PAR_I;
      Par = Params->Items[ParIndex];
      EditEnter();
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_OUT) //кнопка OUT ON/OFF - выход из редактирования
  {
    Analog->AdcI->Force();
    Analog->AdcV->Force();
    if(Edit)
    {
      EditExit();
    }
    return; //код кнопки не сбрасывется
  }
  if(msg == KBD_ENC)
  {
    if(Edit)
    {
      EditExit();
    }
    else
    {
      ParIndex = PAR_V;
      Par = Params->Items[ParIndex];
      EditEnter();
      //старый вариант - вход в меню SETUP по нажатию:      
      //ParIndex = 0;
      //MnuIndex = MNU_SETUP;
    }
    msg = KBD_NOP;
    return;
  }
  //новый вариант - вход в меню SETUP по удержанию:
  //меню - дополнительный функционал устройста, его
  //нужно отдельить от основного функционала.
  //Поэтому короткое нажатие лучше использовать как
  //альтернативный вход в редактирование V.
  if(msg == KBD_ENCH)
  {
    ParIndex = 0;
    MnuIndex = MNU_SETUP;
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETVH)
  {
    EditEscape();
    ParIndex = PAR_OVP;  //активный параметр PAR_OVP
    MnuIndex = MNU_PROT; //переход в меню MNU_PROT
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETIH)
  {
    EditEscape();
    ParIndex = PAR_OCP;
    MnuIndex = MNU_PROT;
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETVI)
  {
    Power = !Power;
    Analog->AdcI->Force();
    Analog->AdcV->Force();
    msg = KBD_NOP;
    return;
  }
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuMain::OnEncoder(int8_t &step)
{
  if(Data->SetupData->Items[PAR_LOCK]->Value == ON)
  {
    //TODO: нужно ли выводить меню "Lock" при повороте
    //заблокированного энкодера?
    Sound->Beep();
    ParIndex = 0;
    MnuIndex = MNU_SETUP;
    step = ENC_NOP;
    return;
  }
  if(Edit)
  {
    //редактирование параметра:
    if(Par->Edit(step))
    {
      Par->ShowValue();
      step = ENC_NOP;
    }
    if(Data->SetupData->Items[PAR_CON]->Value == OFF)
    {
      if(ParIndex == PAR_V)
        Analog->DacV->SetValue(Par->Value); //загрузка DAC_V
      if(ParIndex == PAR_I)
        Analog->DacI->SetValue(Par->Value); //загрузка DAC_I
    }
  }
  else
  {
    //вход в редактирование:
    if(Data->SetupData->Items[PAR_TRC]->Value == ON)
    {
      ParIndex = PAR_V;
      Par = Params->Items[ParIndex];
      EditEnter();
      Sound->Beep();
      step = ENC_NOP;
    }
    //step = ENC_NOP; //нет звука выключенного энкодера
  }
}

//---------------------------- Событие таймера: ------------------------------

void TMenuMain::OnTimer(void)
{
  if(Edit)
  {
    if(Data->SetupData->Items[PAR_CON]->Value == OFF)
      EditExit();
        else { EditEscape(); Par->ShowValue(); }
  }
}

//------------------------ Выход из редактирования: --------------------------

void TMenuMain::EditExit(void)
{
  if(Edit)
  {
    if(ParIndex == PAR_V)
    {
      Edit = 0;
      Display->Blink(BLINK_NO);
      Sound->High();
      Data->SaveV(); //сохранение V в кольцевом буфере EEPROM
      Analog->AdcI->Force();
      Analog->AdcV->Force();
    }
    else
    {
      TMenuItem::EditExit();
    }
    if(ParIndex == PAR_V)
      Analog->DacV->SetValue(Par->Value); //загрузка DAC_V
    if(ParIndex == PAR_I)
      Analog->DacI->SetValue(Par->Value); //загрузка DAC_I
  }
}

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuPreset: -----------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

//ParIndex = 0..(PRESETS - 1) - чтение пресета (Edit = 0)
//ParIndex = PRESETS..(2 * PRESETS - 1) - сохранение пресета (Edit = 1)

void TMenuPreset::Init(void)
{
  if(ParIndex < PRESETS) { Edit = 0; } //чтение пресета
    else { Edit = 1; ParIndex -= PRESETS; }  //запись пресета
  EditEnter();
  Timeout = TIMEOUT_SETUP;
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuPreset::OnKeyboard(KeyMsg_t &msg)
{
  if(msg == KBD_ENC)
  {
    if(ParIndex == PRESETS)
    {
      Sound->High();
      EditEscape();
    }
    else
    {
      EditExit();
    }
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
  }
  if((msg == KBD_SETV) || (msg == KBD_SETI))
  {
    EditEscape();
    Sound->High();
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
  }
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuPreset::OnEncoder(int8_t &step)
{
  if(step > 0 && ParIndex < PRESETS)
  {
    ParIndex++;
    if(ParIndex < PRESETS)
    {
      EditEscape();
      EditEnter();
    }
    else
    {
      LoadParam(Data->SetupData->Items[PAR_ESC]);
      Display->Blink(BLINK_NO);
    }
    step = ENC_NOP;
  }
  if(step < 0 && ParIndex > 0)
  {
    ParIndex--;
    EditEscape();
    EditEnter();
    step = ENC_NOP;
  }
}

//------------------------- Вход в редактирование: ---------------------------

void TMenuPreset::EditEnter(void)
{
  BackupV = Params->Items[PAR_V]->Value;
  BackupI = Params->Items[PAR_I]->Value;
  Data->ReadPreset(ParIndex);
  Display->Blink(BLINK_VI);
  Params->Items[PAR_V]->ShowValue();
  Params->Items[PAR_I]->ShowValue();
}

//------------------------ Выход из редактирования: --------------------------

void TMenuPreset::EditExit(void)
{
  if(Edit)
  {
    Params->Items[PAR_V]->Value = BackupV;
    Params->Items[PAR_I]->Value = BackupI;
    Data->SavePreset(ParIndex);
  }
  else
  {
    Data->OutOn = 0; //выключение выхода при чтении пресета
    //TODO: или состояние выхода не менять?
  }
}

//----------------- Выход из редактирования без сохранения: ------------------

void TMenuPreset::EditEscape(void)
{
  Params->Items[PAR_V]->Value = BackupV;
  Params->Items[PAR_I]->Value = BackupI;
}

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuSetup: ------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuSetup::Init(void)
{
  Edit = 0;
  LoadParam(Params->Items[ParIndex]);
  if((ParIndex == PAR_LOCK) && (Par->Value == ON))
    Timeout = TIMEOUT_LOCK;
      else Timeout = TIMEOUT_SETUP;
}

//---------------------------- Обновление меню: ------------------------------

void TMenuSetup::Execute(void)
{
  if(ParIndex == PAR_HST)
  {
    if(Analog->UpdTemp())
    {
      Par->Value = Analog->GetTemp();
      Par->ShowValue();
    }
  }
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuSetup::OnKeyboard(KeyMsg_t &msg)
{
  if((Data->SetupData->Items[PAR_LOCK]->Value == ON) && !Edit)
  {
    if(msg == KBD_ENC)
    {
      msg = KBD_NOP;
      return;
    }
    if((msg == KBD_SETV) ||
       (msg == KBD_SETI) ||
       (msg == KBD_FINE))
    {
      MnuIndex = MNU_MAIN;
      msg = KBD_NOP;
      return;
    }
    if(msg == KBD_ENCH)
    {
      Timeout = TIMEOUT_SETUP;
      EditEnter();
      msg = KBD_NOP;
      return;
    }
  }
  if(msg == KBD_ENC)
  {
    if(ParIndex == PAR_CALL)
    {
      ParIndex = 0;
      MnuIndex = MNU_PRESET;
      msg = KBD_NOP;
      return;
    }
    if(ParIndex == PAR_STOR)
    {
      ParIndex = PRESETS;
      MnuIndex = MNU_PRESET;
      msg = KBD_NOP;
      return;
    }
    if(ParIndex == PAR_DEF && Par->Value == YES)
    { 
      Display->Off();
      Data->Apply(ParIndex);
      Display->On();
      MnuIndex = MNU_MAIN;
      msg = KBD_NOP;
      return;
    }
    if(ParIndex == PAR_CAL && Par->Value == YES)
    {
      ParIndex = 0;
      MnuIndex = MNU_CALIB;
      msg = KBD_NOP;
      return;
    }
    if(ParIndex == PAR_ESC)
    {
      MnuIndex = MNU_MAIN;
      msg = KBD_NOP;
      return;
    }
    //выход, если редактирование запрещено:
    if(Par->Min == Par->Max) return;
    //вход в редактирование:
    if(!Edit)
    {
      EditEnter();
    }
    //выход из редактирования:
    else
    {
      EditExit();
      Data->Apply(ParIndex);
      if(ParIndex == PAR_LOCK)
        MnuIndex = MNU_MAIN;
    }
    msg = KBD_NOP;
  }
  if((msg == KBD_SETV) || (msg == KBD_SETI))
  {
    EditExit();
    Data->Apply(ParIndex);
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
  }
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuSetup::OnEncoder(int8_t &step)
{
  if((Data->SetupData->Items[PAR_LOCK]->Value == ON) &&
     !(Edit && (ParIndex == PAR_LOCK)))
  {
    return;
  }
  if(Edit)
  {
    if(Par->Edit(step))
    {
      Par->ShowValue();
      step = ENC_NOP;
    }
  }
  else  
  {
    if(step > 0 && ParIndex < Params->ItemsCount - 1)
    {
      LoadParam(Params->Items[++ParIndex]);
      step = ENC_NOP;
    }
    if(step < 0 && ParIndex > 0)
    {
      LoadParam(Params->Items[--ParIndex]);
      step = ENC_NOP;
    }
  }
  if(ParIndex == PAR_HST)
  {
    Analog->ForceTemp();
  }
}

//----------------------------------------------------------------------------
//---------------------------- Класс TMenuProt: ------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

//ParIndex = PAR_OVP - установка порога OVP
//ParIndex = PAR_OCP - установка порога OCP
//ParIndex = PAR_OPP - установка порога OPP
//ParIndex = PAR_OVP + PROT_FLAG - индикация сработки OVP
//ParIndex = PAR_OCP + PROT_FLAG - индикация сработки OCP
//ParIndex = PAR_OPP + PROT_FLAG - индикация сработки OPP
//ParIndex = PAR_OTP + PROT_FLAG - индикация сработки OTP

void TMenuProt::Init(void)
{
  Prot = ParIndex & PROT_FLAG;
  ParIndex &= ~PROT_FLAG;
  LoadParam(Params->Items[ParIndex]);
  if(ParIndex != PAR_OTP)
    EditEnter();
  if(Prot)
  {
    if(ParIndex == PAR_OTP)
      Display->Blink(BLINK_V);
        else Display->Blink(BLINK_VI);
    Timeout = 0;
  }
  else
  {
    Timeout = TIMEOUT_SETUP;
  }
}

//---------------------------- Обновление меню: ------------------------------

void TMenuProt::Execute(void)
{
  if(ParIndex == PAR_OTP)
  {
    if(Analog->GetProtSt() & PR_OTP)  
    {
      if(Analog->UpdTemp())
      {
        Par->Value = Analog->GetTemp();
        Par->ShowValue();
      }
    }
    else
    {
      MnuIndex = MNU_MAIN;
    }
  }
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuProt::OnKeyboard(KeyMsg_t &msg)
{
  if(ParIndex == PAR_OTP)
  {
    if(msg == KBD_ENC)
    {
      ParIndex = 0;
      MnuIndex = MNU_SETUP; //переход в меню MNU_SETUP
      msg = KBD_NOP;
    }
    return;
  }
  if(msg == KBD_SETV)
  {
    if(ParIndex != PAR_OVP)
    {
      EditExit();
      ParIndex = PAR_OVP;
      LoadParam(Params->Items[ParIndex]);
      EditEnter();
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETI)
  { 
    if(ParIndex != PAR_OCP)
    {
      EditExit();
      ParIndex = PAR_OCP;
      LoadParam(Params->Items[ParIndex]);
      EditEnter();
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETVI)
  { 
    if(ParIndex != PAR_OPP)
    {
      EditExit();
      ParIndex = PAR_OPP;
      LoadParam(Params->Items[ParIndex]);
      EditEnter();
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_ENC)
  {
    if(!Prot && (ParIndex < Params->ItemsCount - 2))
    {
      EditExit();
      ParIndex++;
      LoadParam(Params->Items[ParIndex]);
      EditEnter();
    }
    else
    {
      EditExit();
      Analog->ClrProtSt();
      MnuIndex = MNU_MAIN;
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_OUT)
  {
    if(Prot)
    {
      EditExit();
      Analog->ClrProtSt();
      MnuIndex = MNU_MAIN;
    }
    return; //код кнопки не сбрасывется
  }
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuProt::OnEncoder(int8_t &step)
{
  if(ParIndex == PAR_OTP) return;
  if(Edit)
  {
    if(Par->Edit(step))
    {
      Par->ShowValue();
      step = ENC_NOP;
    }
  }
  IndicateOff();
}

//----------------------------- Индикация OFF: -------------------------------

void TMenuProt::IndicateOff(void)
{
  if(Par->Value == Par->Max)
  {
    Display->SetPos((ParIndex == PAR_OVP)? 0 : 1, 0);
    Display->PutString(" OFF");
  }
}

//------------------------- Вход в редактирование: ---------------------------

void TMenuProt::EditEnter(void)
{
  TMenuItem::EditEnter();
  IndicateOff();
}

//----------------------------------------------------------------------------
//---------------------------- Класс TMenuTop: -------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuTop::Init(void)
{
  LoadParam(Params->Items[ParIndex]);
  EditEnter();
  Timeout = TIMEOUT_SETUP;
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuTop::OnKeyboard(KeyMsg_t &msg)
{
  //для входа в меню MNU_TOP кнопку надо отпустить
  //при появлении индикации меню,
  //удержание любой кнопки - выход в MNU_MAIN:
  if(msg & KBD_HOLD)
  {
    EditEscape();
    MnuIndex = MNU_MAIN;
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETV)
  {
    if(ParIndex != PAR_MAXV)
    {
      EditExit();
      Data->TrimParamsLimits();
      ParIndex = PAR_MAXV;
      LoadParam(Params->Items[ParIndex]);
      EditEnter();
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETI)
  {
    if(ParIndex != PAR_MAXI)
    {
      EditExit();
      Data->TrimParamsLimits();
      ParIndex = PAR_MAXI;
      LoadParam(Params->Items[ParIndex]);
      EditEnter();
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_SETVI)
  {
    if(ParIndex != PAR_MAXP)
    {
      EditExit();
      Data->TrimParamsLimits();
      ParIndex = PAR_MAXP;
      LoadParam(Params->Items[ParIndex]);
      EditEnter();
    }
    msg = KBD_NOP;
    return;
  }
  if(msg == KBD_ENC)
  {
    EditExit();
    Data->TrimParamsLimits();
    if(ParIndex < Params->ItemsCount - 1)
    {
      LoadParam(Params->Items[++ParIndex]);
      EditEnter();
    }
    else
    {
      MnuIndex = MNU_MAIN;
    }
    msg = KBD_NOP;
    return;
  }
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuTop::OnEncoder(int8_t &step)
{
  if(Edit)
  {
    if(Par->Edit(step))
    {
      Par->ShowValue();
      step = ENC_NOP;
    }
  }
}

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuCalib: ------------------------------
//----------------------------------------------------------------------------

//-------------------------- Инициализация меню: -----------------------------

void TMenuCalib::Init(void)
{
  Edit = 1;
  Timeout = 0;                        //нет выхода по таймеру
  UpdateCV = 0;
  UpdateCI = 0;
  Analog->OutControl(0);              //выключение выхода
  Analog->TrimParamsLimits();         //коррекция значений согласно Top
  LoadParam(Params->Items[ParIndex]);
  Display->Blink(BLINK_V);
}

//-------------------------- Событие энкодера: -------------------------------

void TMenuCalib::OnEncoder(int8_t &step)
{
  //редактирование параметра:
  if(Edit)
  {
    if(Par->Edit(step))
    {
      Par->ShowValue();
      step = ENC_NOP;
    }
  }
  //коррекция кода при редактировании точек калибровки:
  if(ParIndex == CAL_VP1)
    Params->Items[CAL_VC1]->Value = Analog->DacV->ValueToCode(Par->Value);
  if(ParIndex == CAL_VP2)
    Params->Items[CAL_VC2]->Value = Analog->DacV->ValueToCode(Par->Value);
  if(ParIndex == CAL_IP1)
    Params->Items[CAL_IC1]->Value = Analog->DacI->ValueToCode(Par->Value);
  if(ParIndex == CAL_IP2)
    Params->Items[CAL_IC2]->Value = Analog->DacI->ValueToCode(Par->Value);
  //загрузка DAC:
  if(Par->Type == PT_VC)
    { Analog->DacV->SetCode(Par->Value); UpdateCV = 1; }
  if(Par->Type == PT_IC)
    { Analog->DacI->SetCode(Par->Value); UpdateCI = 1; }
}

//-------------------------- Событие клавиатуры: -----------------------------

void TMenuCalib::OnKeyboard(KeyMsg_t &msg)
{
  if(msg == KBD_SETV)
  {
    if(ParIndex > 0)
    {
      Apply(ParIndex);
      ParIndex--;       //ParIndex = CAL_VP1;
      LoadParam(Params->Items[ParIndex]);
      if(ParIndex == CAL_IC2)
        Params->Items[CAL_IP2]->ShowValue();
      msg = KBD_NOP;
    }
  }
  if(msg == KBD_SETI)
  {
    if(ParIndex < CAL_STR)
    {
      Apply(ParIndex);
      ParIndex++;       //ParIndex = CAL_IP1;
      LoadParam(Params->Items[ParIndex]);
      msg = KBD_NOP;
    }
  }
  if(msg == KBD_ENC)
  {
    if(ParIndex < CAL_STR) //параметры измерителя не участвуют
    {
      Apply(ParIndex);
      ParIndex++;
      LoadParam(Params->Items[ParIndex]);
      msg = KBD_NOP;
    }
    else //запрос сохранения
    {
      Sound->High();
      if(Par->Value == YES)
      {
        Display->Off();
        Analog->CalibData->SaveToEeprom();
        Display->On();
      }
      else if(Par->Value == DEFAULT)
      {
        Display->Off();
        Analog->CalibData->LoadDefaults();
        Analog->CalibData->SaveToEeprom();
        Display->On();
        Analog->CalibAll();
      }
      else
      {
        Analog->CalibData->ReadFromEeprom();
        Analog->CalibAll();
      }
      MnuIndex = MNU_MAIN;
      msg = KBD_NOP;
    }
  }
  //пост-обработка обработанных кнопок:
  if(msg == KBD_NOP)
  {
    //включение мигания редактируемой величины:
    if((Par->Type == PT_V) ||
       (Par->Type == PT_IC))
      Display->Blink(BLINK_V);
    
    if((Par->Type == PT_I) ||
       (Par->Type == PT_VC) ||
       (Par->Type == PT_NYDEF))
      Display->Blink(BLINK_I);
    
    //загрузка DAC и включение выхода:
    if(Par->Type == PT_VC)
    {
      Analog->DacV->SetCode(Par->Value);
      Analog->DacI->SetCode(DAC_CAL_CODE);
      Analog->OutControl(1); //включение выхода
    }
    else if (Par->Type == PT_IC)
    {
      Analog->DacV->SetCode(DAC_CAL_CODE);
      Analog->DacI->SetCode(Par->Value);
      Analog->OutControl(1); //включение выхода
    }
    else
    {
      Analog->OutControl(0); //выключение выхода
    }
  }
  if(msg == KBD_OUT)
  {
    if((Par->Type != PT_IC) && (Par->Type != PT_VC))
      msg = KBD_ERROR; //запрет операции
  }
}

//------------------------- Применение калибровки: ---------------------------

//Измеритель калибруется только в том случае,
//если значения C1 или C2 менялись.
//TODO: Проверять еще состояние CC/CV и OUT ON/OFF
//(не портить калибровку, если был не тот режим).

void TMenuCalib::Apply(char p)
{
  if(p == CAL_VC1)
  {
    Analog->CalibDacV();
    if(UpdateCV) Analog->CalibAdcV(CAL_VM1);
  }
  if(p == CAL_VC2)
  {
    Analog->CalibDacV();
    if(UpdateCV) Analog->CalibAdcV(CAL_VM2);
  }
  if(p == CAL_IC1)
  {
    Analog->CalibDacI();
    if(UpdateCI) Analog->CalibAdcI(CAL_IM1);
  }
  if(p == CAL_IC2)
  {
    Analog->CalibDacI();
    if(UpdateCI) Analog->CalibAdcI(CAL_IM2);
  }
}

//----------------------------------------------------------------------------
//---------------------------- Класс TMenuItems: -----------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TMenuItems::TMenuItems(char max) : TList(max)
{
  AddItem(new TMenuInfo(NULL));               //MNU_INFO
  AddItem(new TMenuMain(Data->MainData));     //MNU_MAIN
  AddItem(new TMenuSetup(Data->SetupData));   //MNU_SETUP
  AddItem(new TMenuPreset(Data->MainData));   //MNU_PRESET
  AddItem(new TMenuProt(Data->ProtData));     //MNU_PROT
  AddItem(new TMenuTop(Data->TopData));       //MNU_TOP
  AddItem(new TMenuCalib(Analog->CalibData)); //MNU_CALIB
}

//---------------------- Переход в меню по индексу: --------------------------

void TMenuItems::SelectMenu(Menu_t mnu, char par)
{
  SelectedMenu = Items[mnu];
  SelectedMenu->MnuIndex = mnu;
  SelectedMenu->ParIndex = par;
  SelectedMenu->Init();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

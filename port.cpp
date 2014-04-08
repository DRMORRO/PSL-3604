//----------------------------------------------------------------------------

//Модуль поддержки порта

//----------------------------------------------------------------------------

#include "main.h"
#include "port.h"
#include "display.h"
#include "data.h"
#include "analog.h"
#include "fan.h"

//----------------------------------------------------------------------------
//------------------------------ Класс TPort ---------------------------------
//----------------------------------------------------------------------------

//----------------------------- Конструктор: ---------------------------------

TPort::TPort(void)
{
  WakePort = new TWakePort(BAUD_RATE, FRAME_SIZE);
}

//-------------------------- Выполнение команд: ------------------------------

void TPort::Execute(void)
{
  char Command = WakePort->GetCmd(); //чтение кода принятой команды
  if(Command != CMD_NOP)             //если есть команда, выполнение
  {
    switch(Command)
    {
    //стандартные команды:  
    //обработка ошибки
    case CMD_ERR:                    
      {
        WakePort->AddByte(ERR_TX);
        break;
      }
    //эхо
    case CMD_ECHO:                   
      {
        char cnt = WakePort->GetRxCount();
        for(char i = 0; i < cnt; i++)
          WakePort->AddByte(WakePort->GetByte());
        break;
      }
    //чтение иформации об устройстве
    case CMD_INFO:                   
      {
        char Info[] = {DEVICE_NAME};
        char *s = Info;
        do WakePort->AddByte(*s++);
          while(*s); 
        break;
      }
    //специальные команды:
    //TODO: местная индикация при управлении от компьютера?
    //установка напряжения и тока
    case CMD_SET_VI:
      {
        Data->MainData->Items[PAR_V]->Value = WakePort->GetWord();
        Data->MainData->Items[PAR_I]->Value = WakePort->GetWord();
        Data->MainData->Items[PAR_V]->Validate();
        Data->MainData->Items[PAR_I]->Validate();
        Data->OutOn = WakePort->GetByte();
        //Data->SaveV();
        //Data->MainData->SaveToEeprom(PAR_I);
        Analog->ClrProtSt();
        Data->SetVI();
        WakePort->AddByte(ERR_NO);
        break;
      }
    //чтение установленного напряжения и тока
    case CMD_GET_VI:
      {
        WakePort->AddByte(ERR_NO);
        WakePort->AddWord(Data->MainData->Items[PAR_V]->Value);
        WakePort->AddWord(Data->MainData->Items[PAR_I]->Value);
        break;
      }
    //чтение статуса источника
    case CMD_GET_STAT:
      {
        WakePort->AddByte(ERR_NO);
        char state, s = 0;
        if(Analog->OutState()) s |= 0x01;
        state = Analog->GetCvCcSt();
        if(state & PS_CV) s |= 0x02;
        if(state & PS_CC) s |= 0x04;
        state = Analog->GetProtSt();
        if(state & PR_OVP) s |= 0x08;
        if(state & PR_OCP) s |= 0x10;
        if(state & PR_OPP) s |= 0x20;
        if(state & PR_OTP) s |= 0x40;
        WakePort->AddByte(s);
        break;
      }
    //чтение среднего измеренного напряжения и тока
    case CMD_GET_VI_AVG:
      {
        WakePort->AddByte(ERR_NO);
        WakePort->AddWord(Analog->AdcV->Value);
        WakePort->AddWord(Analog->AdcI->Value);
        break;
      }
    //чтение мгновенного измеренного напряжения и тока
    case CMD_GET_VI_FAST:
      {
        WakePort->AddByte(ERR_NO);
        WakePort->AddWord(Analog->AdcV->FastValue);
        WakePort->AddWord(Analog->AdcI->FastValue);
        break;
      }
    //установка макс. напряжения, тока и мощности
    case CMD_SET_VIP_MAX:
      {
        Data->TopData->Items[PAR_MAXV]->Value = WakePort->GetWord();
        Data->TopData->Items[PAR_MAXI]->Value = WakePort->GetWord();
        Data->TopData->Items[PAR_MAXP]->Value = WakePort->GetWord();
        Data->TopData->Items[PAR_MAXV]->Validate();
        Data->TopData->Items[PAR_MAXI]->Validate();
        Data->TopData->Items[PAR_MAXP]->Validate();
        Display->Off();
        Data->TopData->SaveToEeprom(PAR_MAXV);
        Data->TopData->SaveToEeprom(PAR_MAXI);
        Data->TopData->SaveToEeprom(PAR_MAXP);
        Display->On();
        Data->TrimParamsLimits();
        WakePort->AddByte(ERR_NO);
        break;
      }
    //чтение макс. напряжения, тока и мощности
    case CMD_GET_VIP_MAX:
      {
        WakePort->AddByte(ERR_NO);
        WakePort->AddWord(Data->TopData->Items[PAR_MAXV]->Value);
        WakePort->AddWord(Data->TopData->Items[PAR_MAXI]->Value);
        WakePort->AddWord(Data->TopData->Items[PAR_MAXP]->Value);
        break;
      }
    //установка напряжения и тока защиты
    case CMD_SET_PROT:
      {
        Data->ProtData->Items[PAR_OVP]->Value = WakePort->GetWord();
        Data->ProtData->Items[PAR_OCP]->Value = WakePort->GetWord();
        Data->ProtData->Items[PAR_OPP]->Value = WakePort->GetWord();
        Data->ProtData->Items[PAR_OVP]->Validate();
        Data->ProtData->Items[PAR_OCP]->Validate();
        Data->ProtData->Items[PAR_OPP]->Validate();
        Display->Off();
        Data->ProtData->SaveToEeprom(PAR_OVP);
        Data->ProtData->SaveToEeprom(PAR_OCP);
        Data->ProtData->SaveToEeprom(PAR_OPP);
        Display->On();
        Analog->ClrProtSt();
        WakePort->AddByte(ERR_NO);
        break;
      }
    //чтение напряжения и тока защиты
    case CMD_GET_PROT:
      {
        WakePort->AddByte(ERR_NO);
        WakePort->AddWord(Data->ProtData->Items[PAR_OVP]->Value);
        WakePort->AddWord(Data->ProtData->Items[PAR_OCP]->Value);
        WakePort->AddWord(Data->ProtData->Items[PAR_OPP]->Value);
        break;
      }
    //запись пресета
    case CMD_SET_PRE:
      {
        char n = WakePort->GetByte();
        if(n < PRESETS)
        {
          uint16_t v = Data->MainData->Items[PAR_V]->Value;
          uint16_t i = Data->MainData->Items[PAR_I]->Value;
          Data->MainData->Items[PAR_V]->Value = WakePort->GetWord();
          Data->MainData->Items[PAR_I]->Value = WakePort->GetWord();
          Data->MainData->Items[PAR_V]->Validate();
          Data->MainData->Items[PAR_I]->Validate();
          Data->SavePreset(n);
          Data->MainData->Items[PAR_V]->Value = v;
          Data->MainData->Items[PAR_I]->Value = i;
          WakePort->AddByte(ERR_NO);
        }
        else
        {
          WakePort->AddByte(ERR_PA);
        }
        break;
      }
    //чтение пресета
    case CMD_GET_PRE:
      {
        char n = WakePort->GetByte();
        if(n < PRESETS)
        {
          WakePort->AddByte(ERR_NO);
          uint16_t v = Data->MainData->Items[PAR_V]->Value;
          uint16_t i = Data->MainData->Items[PAR_I]->Value;
          Data->ReadPreset(n);
          WakePort->AddWord(Data->MainData->Items[PAR_V]->Value);
          WakePort->AddWord(Data->MainData->Items[PAR_I]->Value);
          Data->MainData->Items[PAR_V]->Value = v;
          Data->MainData->Items[PAR_I]->Value = i;
        }
        else
        {
          WakePort->AddByte(ERR_PA);
        }
        break;
      }
    //установка параметра
    case CMD_SET_PAR:
      {
        char n = WakePort->GetByte();
        if(n < PARS_SETUP)
        {
          Data->SetupData->Items[n]->Value = WakePort->GetWord();
          Data->SetupData->Items[n]->Validate();
          Data->SetupData->SaveToEeprom(n);
          if(n == PAR_DEF)
            Display->Off();
          Data->Apply(n);
          Display->On();
          WakePort->AddByte(ERR_NO);
        }
        else
        {
          WakePort->AddByte(ERR_PA);
        }
        break;
      }
    //чтение параметра
    case CMD_GET_PAR:
      {
        char n = WakePort->GetByte();
        if(n < PARS_SETUP)
        {
          WakePort->AddByte(ERR_NO);
          WakePort->AddWord(Data->SetupData->Items[n]->Value);
        }
        else
        {
          WakePort->AddByte(ERR_PA);
        }
        break;
      }
    //чтение скорости вентилятора и температуры
    case CMD_GET_FAN:
      {
        WakePort->AddByte(ERR_NO);
        WakePort->AddByte(Analog->GetSpeed());
        WakePort->AddWord(Analog->GetTemp());
        break;
      }
    //установка кода ЦАП
    case CMD_SET_DAC:
      {
        uint16_t cv = WakePort->GetWord();
        uint16_t ci = WakePort->GetWord();
        if(cv > DACM) cv = DACM;
        if(ci > DACM) ci = DACM;
        Analog->DacV->SetCode(cv);
        Analog->DacI->SetCode(ci);
        WakePort->AddByte(ERR_NO);
        break;
      }
    //чтение кода АЦП
    case CMD_GET_ADC:
      {
        WakePort->AddByte(ERR_NO);
        WakePort->AddWord(Analog->AdcV->Code);
        WakePort->AddWord(Analog->AdcI->Code);
        break;
      }
    //установка калибровочного коэффициента
    case CMD_SET_CAL:
      {
        char n = WakePort->GetByte();
        if(n < CAL_CNT)
        {
          Analog->CalibData->Items[n]->Value = WakePort->GetWord();
          Analog->CalibData->Items[n]->Validate();
          Analog->CalibData->SaveToEeprom(n);
          Analog->CalibAll();
          WakePort->AddByte(ERR_NO);
        }
        else
        {
          WakePort->AddByte(ERR_PA);
        }
        break;
      }
    //чтение калибровочного коэффициента
    case CMD_GET_CAL:
      {
        char n = WakePort->GetByte();
        if(n < CAL_CNT)
        {
          WakePort->AddByte(ERR_NO);
          WakePort->AddWord(Analog->CalibData->Items[n]->Value);
        }
        else
        {
          WakePort->AddByte(ERR_PA);
        }
        break;
      }
    //неизвестная команда
    default: 
      {
        WakePort->AddByte(ERR_PA);
      }      
    }
    WakePort->StartTx(Command);
  }
}

//----------------------------------------------------------------------------

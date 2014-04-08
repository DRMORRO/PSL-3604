//----------------------------------------------------------------------------

//Модуль поддержки порта, заголовочный файл

//----------------------------------------------------------------------------

#ifndef PORT_H
#define PORT_H

#include "wakeport.h"

//----------------------------- Константы: -----------------------------------

#define DEVICE_NAME "PSL-3604" //имя устройства
#define BAUD_RATE       19200  //скорость обмена, бод
#define FRAME_SIZE         32  //максимальный размер фрейма, байт

//----------------------------------------------------------------------------
//------------------------------ Класс TPort ---------------------------------
//----------------------------------------------------------------------------

class TPort
{
private:
public:
  TWakePort *WakePort;
  TPort(void);
  void Execute(void);
};

//----------------------------------------------------------------------------
//----------------------------- Коды команд: ---------------------------------
//----------------------------------------------------------------------------

#define CMD_SET_VI 6 //установка напряжения и тока

  //TX: word V, word I, byte S
  //RX: byte Err

  //V = 0..VMAX - напряжение, x0.01 В
  //I = 0..IMAX - ток, x0.001 А
  //S = 0 - выход выключен, 1 - выход включен
  //Err = ERR_NO

#define CMD_GET_VI 7 //чтение установленного напряжения и тока

  //TX:
  //RX: byte Err, word V, word I

  //V = 0..VMAX - напряжение, x0.01 В
  //I = 0..IMAX - ток, x0.001 А
  //Err = ERR_NO

#define CMD_GET_STAT 8 //чтение статуса источника

  //TX:
  //RX: byte Err, byte S

  //S.0 = 1 - выход включен
  //S.1 = 1 - CV
  //S.2 = 1 - CC
  //S.3 = 1 - OVP
  //S.4 = 1 - OCP
  //S.5 = 1 - OPP
  //S.6 = 1 - OTP
  //Err = ERR_NO

#define CMD_GET_VI_AVG 9 //чтение среднего измеренного напряжения и тока

  //TX:
  //RX: byte Err, word VA, word IA

  //VA = 0..VMAX - напряжение, x0.01 В
  //IA = 0..IMAX - ток, x0.001 А
  //Err = ERR_NO

#define CMD_GET_VI_FAST 10 //чтение мгновенного измеренного напряжения и тока

  //TX:
  //RX: byte Err, word VF, word IF

  //VF = 0..VMAX - напряжение, x0.01 В
  //IF = 0..IMAX - ток, x0.001 А
  //Err = ERR_NO

#define CMD_SET_VIP_MAX 11 //установка макс. напряжения, тока и мощности

  //TX: word VM, word IM, word PM
  //RX: byte Err

  //VM = 1000..9999 - напряжение, x0.01 В
  //IM = 1000..9999 - ток, x0.001 А
  //PM = 10..9999 - мощность, x0.1 Вт
  //Err = ERR_NO

#define CMD_GET_VIP_MAX 12 //чтение макс. напряжения, тока и мощности

  //TX:
  //RX: byte Err, word VM, word IM, word PM

  //VM = 1000..9999 - напряжение, x0.01 В
  //IM = 1000..9999 - ток, x0.001 А
  //PM = 10..9999 - мощность, x0.1 Вт
  //Err = ERR_NO

#define CMD_SET_PROT 13 //установка порогов защиты

  //TX: word VP, word IP, word PP
  //RX: byte Err

  //VP = 0..VMAX - напряжение, x0.01 В
  //IP = 0..IMAX - ток, x0.001 А
  //PP = 0..PMAX - мощность, x0.1 Вт
  //Err = ERR_NO

#define CMD_GET_PROT 14 //чтение порогов защиты

  //TX:
  //RX: byte Err, word VP, word IP, word PP

  //VP = 0..VMAX - напряжение, x0.01 В
  //IP = 0..IMAX - ток, x0.001 А
  //PP = 0..PMAX - мощность, x0.1 Вт
  //Err = ERR_NO

#define CMD_SET_PRE 15 //запись пресета

  //TX: byte N, word V, word I
  //RX: byte Err

  //N = 0..9 - номер пресета
  //V = 0..VMAX - напряжение, x0.01 В
  //I = 0..IMAX - ток, x0.001 А
  //Err = ERR_NO, ERR_PA

#define CMD_GET_PRE 16 //чтение пресета

  //TX: byte N
  //RX: byte Err, word V, word I

  //N = 0..9 - номер пресета
  //V = 0..VMAX - напряжение, x0.01 В
  //I = 0..IMAX - ток, x0.001 А
  //Err = ERR_NO, ERR_PA

#define CMD_SET_PAR 17 //установка параметра

  //TX: byte N, word P
  //RX: byte Err

  //N - номер параметра (см. таблицу параметров)
  //P - значение параметра (см. таблицу параметров)
  //Err = ERR_NO, ERR_PA

#define CMD_GET_PAR 18 //чтение параметра

  //TX: byte N
  //RX: byte Err, word P

  //N - номер параметра (см. таблицу параметров)
  //P - значение параметра (см. таблицу параметров)
  //Err = ERR_NO, ERR_PA

#define CMD_GET_FAN 19 //чтение скорости вентилятора и температуры

  //TX:
  //RX: byte Err, byte S, word T

  //S = 0..100 - скорость вентилятора, %
  //T = 0..999 - температура, x0.1°C
  //Err = ERR_NO

#define CMD_SET_DAC 20 //установка кода ЦАП

  //TX: word DACV, word DACI
  //RX: byte Err

  //DACV = 0..65520 - код ЦАП напряжения
  //DACI = 0..65520 - код ЦАП тока
  //Err = ERR_NO

#define CMD_GET_ADC 21 //чтение кода АЦП

  //TX: 
  //RX: byte Err, word ADCV, word ADCI

  //ADCV = 0..65520 - код АЦП напряжения
  //ADCI = 0..65520 - код АЦП тока
  //Err = ERR_NO

#define CMD_SET_CAL 22 //установка калибровочного коэффициента

  //TX: byte N, word K
  //RX: byte Err

  //N - номер коэффициента (см. таблицу коэффициентов)
  //K - значение коэффициента (см. таблицу коэффициентов)
  //Err = ERR_NO, ERR_PA

#define CMD_GET_CAL 23 //чтение калибровочного коэффициента

  //TX: byte N
  //RX: byte Err, word K

  //N - номер коэффициента (см. таблицу коэффициентов)
  //K - значение коэффициента (см. таблицу коэффициентов)
  //Err = ERR_NO, ERR_PA

//----------------------------------------------------------------------------

#endif

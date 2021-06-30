//----------------------------------------------------------------------------

//Project:  PSL-3604
//Compiler: IAR EWARM 6.50
//Target:   STM32F100C8T6B

//----------------------------------------------------------------------------

#include "main.h"
#include "control.h"
#include "port.h"

//----------------------------- ����������: ----------------------------------

TControl *Control;
TPort *Port;

//----------------------------------------------------------------------------
//------------------------- �������� ���������: ------------------------------
//----------------------------------------------------------------------------

int main(void)
{
  TSysTimer::Init();        //������������� ���������� �������
  Control = new TControl(); //�������� ������� ����������
  Port = new TPort();       //�������� ������� �����
  TSysTimer::SecReset();    //����� ���������� �������
  
  do                        //�������� ����
  {
    TSysTimer::Sync();      //������������� ��������� ����� � �������� ������
    Control->Execute();     //���������� �������� ����������
    Port->Execute();        //���������� ������ ����������
  }
  while(1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

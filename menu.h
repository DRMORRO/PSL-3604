//----------------------------------------------------------------------------

//Модуль реализации меню, заголовочный файл

//----------------------------------------------------------------------------

#ifndef MENU_H
#define MENU_H

//----------------------------------------------------------------------------

#include "data.h"
#include "keyboard.h"

//------------------------------- Константы: ---------------------------------

enum Menu_t //индекс меню
{
  MNU_INFO,
  MNU_MAIN,
  MNU_SETUP,
  MNU_PRESET,
  MNU_PROT,
  MNU_TOP,
  MNU_CALIB,
  MENUS
};

enum Info_t //индекс параметра для меню INFO
{
  INF_SPLASH,
  INF_ERREEP,
  INFOS
};

//----------------------------------------------------------------------------
//--------------------- Абстрактный класс TMenuItem: -------------------------
//----------------------------------------------------------------------------

class TMenuItem
{
private:
protected:
  TParamList *Params;
  TParam *Par;
  bool Edit;
  uint16_t BackupV;
  void ShowName(void);
  void ShowValue(void);
  virtual void LoadParam(TParam *p);
  virtual void EditEnter(void);
  virtual void EditExit(void);
  virtual void EditEscape(void);
public:
  TMenuItem(TParamList *p) : Params(p) {};
  Menu_t MnuIndex;
  char ParIndex;
  uint16_t Timeout;
  virtual void Init(void) = 0;
  virtual void OnKeyboard(KeyMsg_t &msg) = 0;
  virtual void OnEncoder(int8_t &msg) = 0;
  virtual void OnTimer(void);
  virtual void Execute(void) {};
};

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuInfo: -------------------------------
//----------------------------------------------------------------------------

class TMenuInfo : public TMenuItem
{
private:
public:
  TMenuInfo(TParamList *p) : TMenuItem(p) {};
  virtual void Init(void);
  virtual void OnEncoder(int8_t &msg);
  virtual void OnKeyboard(KeyMsg_t &msg);
};

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuMain: -------------------------------
//----------------------------------------------------------------------------

class TMenuMain : public TMenuItem
{
private:
  bool Power;
protected:
  virtual void EditExit(void);
public:
  TMenuMain(TParamList *p) : TMenuItem(p) {};
  virtual void Init(void);
  virtual void Execute(void);
  virtual void OnEncoder(int8_t &msg);
  virtual void OnKeyboard(KeyMsg_t &msg);
  virtual void OnTimer(void);
};

//----------------------------------------------------------------------------
//-------------------------- Класс TMenuPreset: ------------------------------
//----------------------------------------------------------------------------

class TMenuPreset : public TMenuItem
{
private:
  uint16_t BackupI;
protected:
  virtual void EditEnter(void);
  virtual void EditExit(void);
  virtual void EditEscape(void);
public:
  TMenuPreset(TParamList *p) : TMenuItem(p) {};
  virtual void Init(void);
  virtual void OnEncoder(int8_t &msg);
  virtual void OnKeyboard(KeyMsg_t &msg);
};

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuSetup: ------------------------------
//----------------------------------------------------------------------------

class TMenuSetup : public TMenuItem
{
private:
public:
  TMenuSetup(TParamList *p) : TMenuItem(p) {};
  virtual void Init(void);
  virtual void Execute(void);
  virtual void OnEncoder(int8_t &msg);
  virtual void OnKeyboard(KeyMsg_t &msg);
};

//----------------------------------------------------------------------------
//---------------------------- Класс TMenuProt: ------------------------------
//----------------------------------------------------------------------------

class TMenuProt : public TMenuItem
{
private:
  bool Prot;
  void IndicateOff(void);
protected:  
  virtual void EditEnter(void);
public:
  TMenuProt(TParamList *p) : TMenuItem(p) {};
  virtual void Init(void);
  virtual void Execute(void);
  virtual void OnEncoder(int8_t &msg);
  virtual void OnKeyboard(KeyMsg_t &msg);
};

//----------------------------------------------------------------------------
//---------------------------- Класс TMenuTop: -------------------------------
//----------------------------------------------------------------------------

class TMenuTop : public TMenuItem
{
private:
public:
  TMenuTop(TParamList *p) : TMenuItem(p) {};
  virtual void Init(void);
  virtual void OnEncoder(int8_t &msg);
  virtual void OnKeyboard(KeyMsg_t &msg);
};

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuCalib: ------------------------------
//----------------------------------------------------------------------------

class TMenuCalib : public TMenuItem
{
private:
  void Apply(char p);
  bool UpdateCV;
  bool UpdateCI;
public:
  TMenuCalib(TParamList *p) : TMenuItem(p) {};
  virtual void Init(void);
  virtual void OnEncoder(int8_t &msg);
  virtual void OnKeyboard(KeyMsg_t &msg);
};

//----------------------------------------------------------------------------
//--------------------------- Класс TMenuItems: ------------------------------
//----------------------------------------------------------------------------

class TMenuItems : public TList<TMenuItem>
{
public:
  TMenuItems(char max);
  void SelectMenu(Menu_t mnu, char par = 0);
  TMenuItem *SelectedMenu;
};

//----------------------------------------------------------------------------

#endif

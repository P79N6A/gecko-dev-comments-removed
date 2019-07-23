




































#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include "thread.h"
#include "action.h"

class nsPluginThread : CThread
{
public:
  nsPluginThread(DWORD aP1);
  ~nsPluginThread();

private:
  BOOL init();
  void shut(); 
  void dispatch();

public:
  DWORD callNPP(npapiAction aAction, DWORD aP1=NULL, 
                DWORD aP2=NULL, DWORD aP3=NULL, DWORD aP4=NULL, 
                DWORD aP5=NULL, DWORD aP6=NULL, DWORD aP7=NULL);

private:
  DWORD mP1;
  DWORD mP2;
  DWORD mP3;
  DWORD mP4;
  DWORD mP5;
  DWORD mP6;
  DWORD mP7;
};

#endif 

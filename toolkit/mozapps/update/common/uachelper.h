




































#ifndef _UACHELPER_H_
#define _UACHELPER_H_

class UACHelper
{
public:
  static BOOL IsVistaOrLater();
  static HANDLE OpenUserToken(DWORD sessionID);
  static HANDLE OpenLinkedToken(HANDLE token);
};

#endif

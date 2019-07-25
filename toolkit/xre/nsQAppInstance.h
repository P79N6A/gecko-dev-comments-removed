





































#ifndef nsQAppInstance_h
#define nsQAppInstance_h

#include <QApplication>

class nsQAppInstance
{
public:
  static void AddRef(void);
  static void Release(void);

private:
  static QApplication *sQAppInstance;
  static int sQAppRefCount;
};

#endif 

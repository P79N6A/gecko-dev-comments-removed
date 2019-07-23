





































#ifndef nsQAppInstance_h
#define nsQAppInstance_h

#include <QApplication>

class nsQAppInstance : public QApplication
{
public:
  static void AddRef(void);
  static void Release(void);

private:
  nsQAppInstance(int gArgc, char** gArgv);
  static nsQAppInstance *sQAppInstance;
  static int sQAppRefCount;
};

#endif 

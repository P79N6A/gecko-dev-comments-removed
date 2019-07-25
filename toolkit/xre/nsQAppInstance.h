





































#ifndef nsQAppInstance_h
#define nsQAppInstance_h


extern int    gArgc;
extern char **gArgv;

class QApplication;
class MComponentData;
class nsQAppInstance
{
public:
  static void AddRef(int& aArgc = gArgc,
                     char** aArgv = gArgv,
                     bool aDefaultProcess = false);
  static void Release(void);

private:
  static QApplication *sQAppInstance;
  static MComponentData* sMComponentData;
  static int sQAppRefCount;
};

#endif 

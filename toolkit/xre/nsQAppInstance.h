





#ifndef nsQAppInstance_h
#define nsQAppInstance_h


extern int    gArgc;
extern char **gArgv;

class QGuiApplication;
class nsQAppInstance
{
public:
  static void AddRef(int& aArgc = gArgc,
                     char** aArgv = gArgv,
                     bool aDefaultProcess = false);
  static void Release(void);

private:
  static QGuiApplication *sQAppInstance;
  static int sQAppRefCount;
};

#endif 

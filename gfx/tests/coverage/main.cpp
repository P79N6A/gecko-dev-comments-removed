



































#include "nsIWidget.h"

extern nsresult CoverageTest(int * argc, char **argv);

#if defined(XP_WIN) || defined(XP_OS2)

#include <windows.h>

void main(int argc, char **argv)
{
  int argC = argc;

  CoverageTest(&argC, argv);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, 
    int nCmdShow)
{
  int  argC = 0;
  char ** argv = NULL;

  return(CoverageTest(&argC, argv));
}

#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
int main(int argc, char **argv)
{
  int argC = argc;

  CoverageTest(&argC, argv);

  return 0;
}
#endif

#ifdef XP_MAC
int main(int argc, char **argv)
{
  int argC = argc;

  CoverageTest(&argC, argv);
	
	return 0;
}
#endif


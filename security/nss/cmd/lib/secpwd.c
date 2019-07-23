


































#include "secutil.h"










#ifdef XP_UNIX
#include <termios.h>
#endif

#if defined(XP_UNIX) || defined(XP_BEOS)
#include <unistd.h>  
#endif

#if( defined(_WINDOWS) && !defined(_WIN32_WCE))
#include <conio.h>
#include <io.h>
#define QUIET_FGETS quiet_fgets
static char * quiet_fgets (char *buf, int length, FILE *input);
#else
#define QUIET_FGETS fgets
#endif

static void echoOff(int fd)
{
#if defined(XP_UNIX)
    if (isatty(fd)) {
	struct termios tio;
	tcgetattr(fd, &tio);
	tio.c_lflag &= ~ECHO;
	tcsetattr(fd, TCSAFLUSH, &tio);
    }
#endif
}

static void echoOn(int fd)
{
#if defined(XP_UNIX)
    if (isatty(fd)) {
	struct termios tio;
	tcgetattr(fd, &tio);
	tio.c_lflag |= ECHO;
	tcsetattr(fd, TCSAFLUSH, &tio);
    }
#endif
}

char *SEC_GetPassword(FILE *input, FILE *output, char *prompt,
			       PRBool (*ok)(char *))
{
#if defined(_WINDOWS)
    int isTTY = (input == stdin);
#define echoOn(x)
#define echoOff(x)
#else
    int infd  = fileno(input);
    int isTTY = isatty(infd);
#endif
    char phrase[200] = {'\0'};      

    for (;;) {
	
	if (isTTY) {
	    fprintf(output, "%s", prompt);
            fflush (output);
	    echoOff(infd);
	}

	QUIET_FGETS ( phrase, sizeof(phrase), input);

	if (isTTY) {
	    fprintf(output, "\n");
	    echoOn(infd);
	}

	
	phrase[PORT_Strlen(phrase)-1] = 0;

	
	if (!(*ok)(phrase)) {
	    
	    if (!isTTY) return 0;
	    fprintf(output, "Password must be at least 8 characters long with one or more\n");
	    fprintf(output, "non-alphabetic characters\n");
	    continue;
	}
	return (char*) PORT_Strdup(phrase);
    }
}



PRBool SEC_CheckPassword(char *cp)
{
    int len;
    char *end;

    len = PORT_Strlen(cp);
    if (len < 8) {
	return PR_FALSE;
    }
    end = cp + len;
    while (cp < end) {
	unsigned char ch = *cp++;
	if (!((ch >= 'A') && (ch <= 'Z')) &&
	    !((ch >= 'a') && (ch <= 'z'))) {
	    
	    return PR_TRUE;
	}
    }
    return PR_FALSE;
}

PRBool SEC_BlindCheckPassword(char *cp)
{
    if (cp != NULL) {
	return PR_TRUE;
    }
    return PR_FALSE;
}



#if defined(_WINDOWS)
static char * quiet_fgets (char *buf, int length, FILE *input)
  {
  int c;
  char *end = buf;

  
  memset (buf, 0, length);

  if (!isatty(fileno(input))) {
     return fgets(buf,length,input);
  }

  while (1)
    {
#if defined (_WIN32_WCE)
    c = getchar();	
#else
    c = getch();	
#endif
    if (c == '\b')
      {
      if (end > buf)
        end--;
      }

    else if (--length > 0)
      *end++ = c;

    if (!c || c == '\n' || c == '\r')
      break;
    }

  return buf;
  }
#endif

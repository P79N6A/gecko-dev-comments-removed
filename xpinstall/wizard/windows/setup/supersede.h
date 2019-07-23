




































#ifndef _supersede_h
#define _supersede_h

#include "setup.h"
#include "version.h"

typedef struct structGreVer grever;
struct structGreVer
{
  char      greHomePath[MAX_BUF];
  char      greInstaller[MAX_BUF];
  char      greUserAgent[MAX_BUF_TINY];
  verBlock  version;
  grever    *next;
};

typedef struct structGreInfo greInfo;
struct structGreInfo
{
  char      homePath[MAX_BUF];
  char      installerAppPath[MAX_BUF];
  char      userAgent[MAX_BUF_TINY];
  char      uninstallerAppPath[MAX_BUF];
  verBlock  minVersion;
  verBlock  maxVersion;
  grever    *greSupersedeList;
  grever    *greInstalledList;
  siC       *siCGreComponent;
};

grever            *CreateGVerNode(void);
BOOL              ResolveSupersede(siC *siCObject, greInfo *gre);
void              DeleteGverList(grever *gverHead);

#endif 

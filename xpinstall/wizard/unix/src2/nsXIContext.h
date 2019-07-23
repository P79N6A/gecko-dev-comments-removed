






































#ifndef _NS_XICONTEXT_H_
#define _NS_XICONTEXT_H_

#include "nsLicenseDlg.h"
#include "nsWelcomeDlg.h"
#include "nsSetupTypeDlg.h"
#include "nsComponentsDlg.h"
#include "nsInstallDlg.h"
#include "nsXIOptions.h"
#include "nsINIParser.h"

#define RES_FILE "installer"
#define RES_SECT "String Resources"

class nsXInstaller;

typedef struct _kvpair
{
    char *key;
    char *val;

    struct _kvpair *next;
} kvpair;

class nsXIContext
{
public:
    nsXIContext();
    ~nsXIContext();

    nsXInstaller        *me;




    nsLicenseDlg        *ldlg;
    nsWelcomeDlg        *wdlg;
    nsSetupTypeDlg      *sdlg;
    nsComponentsDlg     *cdlg;
    nsInstallDlg        *idlg;

    nsXIOptions         *opt;




    GtkWidget           *window;    
    GtkWidget           *back;      
    GtkWidget           *next;      
    GtkWidget           *cancel;    
    GtkWidget           *nextLabel;     
    GtkWidget           *backLabel;     
    GtkWidget           *acceptLabel;   
    GtkWidget           *declineLabel;  
    GtkWidget           *installLabel;  
    GtkWidget           *logo;      
    GtkWidget           *mainbox;   
    GtkWidget           *canvas;    
    GtkWidget           *notebook;  

    int                 backID;     
    int                 nextID;     
    int                 cancelID;   
    int                 bMoving;    

    int                 bDone;      





    int     LoadResources();        
    int     ReleaseResources();     
    char    *Res(char *aKey);       

private:
    kvpair  *reslist;               
};

#endif 








































#ifndef __EmbedDownloadMgr_h
#define __EmbedDownloadMgr_h
#include "EmbedPrivate.h"
#include "nsIHelperAppLauncherDialog.h"
#include "nsIMIMEInfo.h"
#include "nsCOMPtr.h"
#include "nsIExternalHelperAppService.h"
#include "nsIRequest.h"
#include "nsILocalFile.h"
#include "nsWeakReference.h"
#define EMBED_DOWNLOADMGR_DESCRIPTION "MicroB Download Manager"
#define EMBED_DOWNLOADMGR_CID {0x53df12a2, 0x1f4a, 0x4382, {0x99, 0x4e, 0xed, 0x62, 0xcf, 0x0d, 0x6b, 0x3a}}
#define FILE_SCHEME "file://"
#define SLASH "/"
class nsIURI;
class nsIFile;
class nsIFactory;
typedef struct _EmbedDownload EmbedDownload;
struct _EmbedDownload
{
  GtkObject * parent;
  GtkWidget* gtkMozEmbedParentWidget;
  char* file_name;                   
  const char *file_name_with_path;        
  const char* server;                
  PRInt64 file_size;                 
  const char* file_type;             
  const char* handler_app;           
  PRInt64 downloaded_size;           
  gboolean isPaused;                 
  gboolean open_with;                
  gboolean started;                  
  nsIHelperAppLauncher* launcher;    
  nsIRequest* request;               
  nsIFile* file_dest;                
  nsIFile* file_target;              
};
class EmbedDownloadMgr : public nsIHelperAppLauncherDialog
{
 public:
  EmbedDownloadMgr();
  virtual ~EmbedDownloadMgr();
  nsresult Init(void);
  NS_DECL_ISUPPORTS
  NS_DECL_NSIHELPERAPPLAUNCHERDIALOG
 private:
  

  NS_METHOD GetDownloadInfo (void);
  nsCOMPtr<nsIHelperAppLauncher> mLauncher;  
  nsCOMPtr<nsISupports> mContext;            
  nsCOMPtr<nsIURI> mUri;                    
  nsCOMPtr<nsIFile> mDestFileTemp;          
  nsCOMPtr<nsIMIMEInfo> mMIMEInfo;           
  nsCAutoString mSpec;                      
};
#endif 







































#ifndef __EMBEDGLOBALHISTORY_h
#define __EMBEDGLOBALHISTORY_h
#include "nsIGlobalHistory2.h"
#include "nsIObserver.h"
#include "EmbedPrivate.h"
#include <prenv.h>
#include <gtk/gtk.h>
#include "nsDocShellCID.h"

#ifdef MOZ_ENABLE_GNOMEVFS
#include <libgnomevfs/gnome-vfs.h>
#define OUTPUT_STREAM GnomeVFSHandle
#define LOCAL_FILE GnomeVFSURI
#else
#define OUTPUT_STREAM nsIOutputStream
#define LOCAL_FILE nsILocalFile
#endif



#define NS_EMBEDGLOBALHISTORY_CID \
  { 0x2f977d51, 0x5485, 0x11d4, \
  { 0x87, 0xe2, 0x00, 0x10, 0xa4, 0xe7, 0x5e, 0xf2 } }
#define EMBED_HISTORY_PREF_EXPIRE_DAYS      "browser.history_expire_days"



class EmbedGlobalHistory: public nsIGlobalHistory2,
                          public nsIObserver
{
    public:
    EmbedGlobalHistory();
    virtual ~EmbedGlobalHistory();
    static EmbedGlobalHistory* GetInstance();
    static void DeleteInstance();
    NS_IMETHOD        Init();
    nsresult GetContentList(GtkMozHistoryItem**, int *count);
    NS_DECL_ISUPPORTS
    NS_DECL_NSIGLOBALHISTORY2
    NS_DECL_NSIOBSERVER
    nsresult RemoveEntries(const PRUnichar *url = nsnull, int time = 0);

    protected:
    enum {
        kFlushModeAppend,      
        kFlushModeFullWrite    
    };



    nsresult          InitFile();



    nsresult          LoadData();





    nsresult          WriteEntryIfWritten(GList *list, OUTPUT_STREAM *file_handle);





    nsresult          WriteEntryIfUnwritten(GList *list, OUTPUT_STREAM *file_handle);




    nsresult          FlushData(PRIntn mode = kFlushModeFullWrite);
 


    nsresult          ResetData();




    nsresult          ReadEntries(LOCAL_FILE *file_uri);




    nsresult          GetEntry(const char *);
    protected:
    OUTPUT_STREAM    *mFileHandle;             
    PRBool            mDataIsLoaded;           
    PRInt32           mEntriesAddedSinceFlush; 
    gchar*            mHistoryFile;            
};

static const PRUint32 kDefaultMaxSize = 1000;
#endif

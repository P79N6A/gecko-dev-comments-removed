








































#include "xptiprivate.h"

struct xptiFileTypeEntry
{
    const char*         name;
    int                 len;
    xptiFileType::Type  type;
};

static const xptiFileTypeEntry g_Entries[] = 
    {
        {".xpt", 4, xptiFileType::XPT},            
        {".zip", 4, xptiFileType::ZIP},            
        {".jar", 4, xptiFileType::ZIP},            
        {nsnull, 0, xptiFileType::UNKNOWN}            
    };


xptiFileType::Type xptiFileType::GetType(const char* name)
{
    NS_ASSERTION(name, "loser!");
    int len = PL_strlen(name);
    for(const xptiFileTypeEntry* p = g_Entries; p->name; p++)
    {
        if(len > p->len && 0 == PL_strcasecmp(p->name, &(name[len - p->len])))
            return p->type;
    }
    return UNKNOWN;        
}        



xptiAutoLog::xptiAutoLog(xptiInterfaceInfoManager* mgr, 
                         nsILocalFile* logfile, PRBool append)
    : mMgr(nsnull), mOldFileDesc(nsnull)
{
    MOZ_COUNT_CTOR(xptiAutoLog);

    if(mgr && logfile)
    {
        PRFileDesc* fd;
        if(NS_SUCCEEDED(logfile->
                    OpenNSPRFileDesc(PR_WRONLY | PR_CREATE_FILE | PR_APPEND |
                                             (append ? 0 : PR_TRUNCATE),
                                             0666, &fd)) && fd)
        {
#ifdef DEBUG
            m_DEBUG_FileDesc = fd;
#endif
            mMgr = mgr;
            mOldFileDesc = mMgr->SetOpenLogFile(fd);
            if(append)
                PR_Seek(fd, 0, PR_SEEK_END);
            WriteTimestamp(fd, "++++ start logging ");

        }
        else
        {
#ifdef DEBUG
        printf("xpti failed to open log file for writing\n");
#endif
        }
    }
}

xptiAutoLog::~xptiAutoLog()
{
    MOZ_COUNT_DTOR(xptiAutoLog);

    if(mMgr)
    {
        PRFileDesc* fd = mMgr->SetOpenLogFile(mOldFileDesc);
        NS_ASSERTION(fd == m_DEBUG_FileDesc, "bad unravel");
        if(fd)
        {
            WriteTimestamp(fd, "---- end logging   ");
            PR_Close(fd);
        }
    }
}

void xptiAutoLog::WriteTimestamp(PRFileDesc* fd, const char* msg)
{
    PRExplodedTime expTime;
    PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &expTime);
    char time[128];
    PR_FormatTimeUSEnglish(time, 128, "%Y-%m-%d-%H:%M:%S", &expTime);
    PR_fprintf(fd, "\n%s %s\n\n", msg, time);
}



nsresult 
xptiCloneLocalFile(nsILocalFile*  aLocalFile,
                   nsILocalFile** aCloneLocalFile)
{
    nsresult rv;
    nsCOMPtr<nsIFile> cloneRaw;
 
    rv = aLocalFile->Clone(getter_AddRefs(cloneRaw));
    if(NS_FAILED(rv))
        return rv;

    return CallQueryInterface(cloneRaw, aCloneLocalFile);
}                        


nsresult 
xptiCloneElementAsLocalFile(nsISupportsArray* aArray, PRUint32 aIndex,
                            nsILocalFile** aLocalFile)
{
    nsresult rv;
    nsCOMPtr<nsILocalFile> original;

    rv = aArray->QueryElementAt(aIndex, NS_GET_IID(nsILocalFile), 
                                getter_AddRefs(original));
    if(NS_FAILED(rv))
        return rv;

    return xptiCloneLocalFile(original, aLocalFile);
}       

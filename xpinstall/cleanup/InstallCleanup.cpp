



































#include "InstallCleanup.h"

int PerformScheduledTasks(HREG reg)
{
    int deleteComplete  = DONE;
    int replaceComplete = DONE;

    deleteComplete  = DeleteScheduledFiles( reg );
    replaceComplete = ReplaceScheduledFiles( reg );
    if ((deleteComplete == DONE) && (replaceComplete == DONE))
        return DONE;
    else
        return TRY_LATER;
}





int ReplaceScheduledFiles( HREG reg )
{
    RKEY    key;
    int rv = DONE;

    
    if (REGERR_OK == NR_RegGetKey(reg,ROOTKEY_PRIVATE,REG_REPLACE_LIST_KEY,&key))
    {
        char keyname[MAXREGNAMELEN];
        char doomedFile[MAXREGPATHLEN];
        char srcFile[MAXREGPATHLEN];

        uint32 bufsize;
        REGENUM state = 0;
        while (REGERR_OK == NR_RegEnumSubkeys( reg, key, &state, 
                               keyname, sizeof(keyname), REGENUM_CHILDREN))
        {
            bufsize = sizeof(srcFile);
            REGERR err1 = NR_RegGetEntry( reg, (RKEY)state,
                               REG_REPLACE_SRCFILE, &srcFile, &bufsize);

            bufsize = sizeof(doomedFile);
            REGERR err2 = NR_RegGetEntry( reg, (RKEY)state,
                               REG_REPLACE_DESTFILE, &doomedFile, &bufsize);

            if ( err1 == REGERR_OK && err2 == REGERR_OK )
            {
                int result = NativeReplaceFile( srcFile, doomedFile );
                if (result == DONE)
                {
                    
                    NR_RegDeleteKey( reg, key, keyname );
                }
            }
        }

        
        state = 0;
        if (REGERR_NOMORE == NR_RegEnumSubkeys( reg, key, &state, keyname,
                                     sizeof(keyname), REGENUM_CHILDREN ))
        {
            NR_RegDeleteKey(reg, ROOTKEY_PRIVATE, REG_REPLACE_LIST_KEY);
            rv = DONE;
        }
        else
        {
            rv = TRY_LATER;
        }
    }
    return rv;
}




int DeleteScheduledFiles( HREG reg )
{
    REGERR  err;
    RKEY    key;
    REGENUM state = 0;
    int rv = DONE;

    
    if (REGERR_OK == NR_RegGetKey(reg,ROOTKEY_PRIVATE,REG_DELETE_LIST_KEY,&key))
    {
        
        

        char    namebuf[MAXREGNAMELEN];
        char    valbuf[MAXREGPATHLEN];

        while (REGERR_OK == NR_RegEnumEntries( reg, key, &state, namebuf,
                                               sizeof(namebuf), 0 ) )
        {
            uint32 bufsize = sizeof(valbuf); 
            err = NR_RegGetEntry( reg, key, namebuf, valbuf, &bufsize );
            if ( err == REGERR_OK )
            {
                rv = NativeDeleteFile(valbuf);
                if (rv == DONE)
                    NR_RegDeleteEntry( reg, key, namebuf);
            }
        }

        
        state = 0;
        err = NR_RegEnumEntries(reg, key, &state, namebuf, sizeof(namebuf), 0);
        if ( err == REGERR_NOMORE )
        {
            NR_RegDeleteKey(reg, ROOTKEY_PRIVATE, REG_DELETE_LIST_KEY);
            rv = DONE;
        }
        else
        {
            rv = TRY_LATER;
        }
    }
    return rv;
}


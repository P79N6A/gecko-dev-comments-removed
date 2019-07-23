







































#ifndef _MOZSTORAGE_H_
#define _MOZSTORAGE_H_











static nsresult
ConvertResultCode(int srv)
{
    switch (srv) {
        case SQLITE_OK:
        case SQLITE_ROW:
        case SQLITE_DONE:
            return NS_OK;
        case SQLITE_CORRUPT:
        case SQLITE_NOTADB:
            return NS_ERROR_FILE_CORRUPTED;
        case SQLITE_PERM:
        case SQLITE_CANTOPEN:
            return NS_ERROR_FILE_ACCESS_DENIED;
        case SQLITE_BUSY:
        case SQLITE_LOCKED:
            return NS_ERROR_FILE_IS_LOCKED;
        case SQLITE_READONLY:
            return NS_ERROR_FILE_READ_ONLY;
        case SQLITE_FULL:
        case SQLITE_TOOBIG:
            return NS_ERROR_FILE_NO_DEVICE_SPACE;
        case SQLITE_NOMEM:
            return NS_ERROR_OUT_OF_MEMORY;
        case SQLITE_MISUSE:
            return NS_ERROR_UNEXPECTED;
        case SQLITE_ABORT:
            return NS_ERROR_ABORT;
    }

    
    return NS_ERROR_FAILURE;
}

#endif 


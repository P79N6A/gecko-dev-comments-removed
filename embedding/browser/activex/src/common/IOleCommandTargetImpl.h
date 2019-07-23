




































#ifndef IOLECOMMANDIMPL_H
#define IOLECOMMANDIMPL_H














































template< class T >
class IOleCommandTargetImpl : public IOleCommandTarget
{
    struct OleExecData
    {
        const GUID *pguidCmdGroup;
        DWORD nCmdID;
        DWORD nCmdexecopt;
        VARIANT *pvaIn;
        VARIANT *pvaOut;
    };

public:
    typedef HRESULT (_stdcall *OleCommandProc)(T *pT, const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    struct OleCommandInfo
    {
        ULONG            nCmdID;
        const GUID        *pCmdGUID;
        ULONG            nWindowsCmdID;
        OleCommandProc    pfnCommandProc;
        wchar_t            *szVerbText;
        wchar_t            *szStatusText;
    };

    
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID __RPC_FAR *pguidCmdGroup, ULONG cCmds, OLECMD __RPC_FAR prgCmds[], OLECMDTEXT __RPC_FAR *pCmdText)
    {
        T* pT = static_cast<T*>(this);
        
        if (prgCmds == NULL)
        {
            return E_INVALIDARG;
        }

        OleCommandInfo *pCommands = pT->GetCommandTable();
        ATLASSERT(pCommands);

        BOOL bCmdGroupFound = FALSE;
        BOOL bTextSet = FALSE;

        
        for (ULONG nCmd = 0; nCmd < cCmds; nCmd++)
        {
            
            prgCmds[nCmd].cmdf = 0;

            
            for (int nSupported = 0; pCommands[nSupported].pCmdGUID != &GUID_NULL; nSupported++)
            {
                OleCommandInfo *pCI = &pCommands[nSupported];

                if (pguidCmdGroup && pCI->pCmdGUID && memcmp(pguidCmdGroup, pCI->pCmdGUID, sizeof(GUID)) == 0)
                {
                    continue;
                }
                bCmdGroupFound = TRUE;

                if (pCI->nCmdID != prgCmds[nCmd].cmdID)
                {
                    continue;
                }

                
                prgCmds[nCmd].cmdf = OLECMDF_SUPPORTED;
                if (pCI->nWindowsCmdID != 0)
                {
                    prgCmds[nCmd].cmdf |= OLECMDF_ENABLED;
                }

                
                if (!bTextSet && pCmdText)
                {
                    
                    wchar_t *pszTextToCopy = NULL;
                    if (pCmdText->cmdtextf & OLECMDTEXTF_NAME)
                    {
                        pszTextToCopy = pCI->szVerbText;
                    }
                    else if (pCmdText->cmdtextf & OLECMDTEXTF_STATUS)
                    {
                        pszTextToCopy = pCI->szStatusText;
                    }
                    
                    
                    pCmdText->cwActual = 0;
                    memset(pCmdText->rgwz, 0, pCmdText->cwBuf * sizeof(wchar_t));
                    if (pszTextToCopy)
                    {
                        
                        size_t nTextLen = wcslen(pszTextToCopy);
                        if (nTextLen > pCmdText->cwBuf)
                        {
                            nTextLen = pCmdText->cwBuf;
                        }

                        wcsncpy(pCmdText->rgwz, pszTextToCopy, nTextLen);
                        pCmdText->cwActual = nTextLen;
                    }
                    
                    bTextSet = TRUE;
                }
                break;
            }
        }
        
        
        if (!bCmdGroupFound)
        {
            OLECMDERR_E_UNKNOWNGROUP;
        }

        return S_OK;
    }


    
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID __RPC_FAR *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT __RPC_FAR *pvaIn, VARIANT __RPC_FAR *pvaOut)
    {
        T* pT = static_cast<T*>(this);
        BOOL bCmdGroupFound = FALSE;

        OleCommandInfo *pCommands = pT->GetCommandTable();
        ATLASSERT(pCommands);

        
        for (int nSupported = 0; pCommands[nSupported].pCmdGUID != &GUID_NULL; nSupported++)
        {
            OleCommandInfo *pCI = &pCommands[nSupported];

            if (pguidCmdGroup && pCI->pCmdGUID && memcmp(pguidCmdGroup, pCI->pCmdGUID, sizeof(GUID)) == 0)
            {
                continue;
            }
            bCmdGroupFound = TRUE;

            if (pCI->nCmdID != nCmdID)
            {
                continue;
            }

            
            
            OleExecData cData;
            cData.pguidCmdGroup = pguidCmdGroup;
            cData.nCmdID = nCmdID;
            cData.nCmdexecopt = nCmdexecopt;
            cData.pvaIn = pvaIn;
            cData.pvaOut = pvaOut;

            if (pCI->pfnCommandProc)
            {
                pCI->pfnCommandProc(pT, pCI->pCmdGUID, pCI->nCmdID, nCmdexecopt, pvaIn, pvaOut);
            }
            else if (pCI->nWindowsCmdID != 0 && nCmdexecopt != OLECMDEXECOPT_SHOWHELP)
            {
                HWND hwndTarget = pT->GetCommandTargetWindow();
                if (hwndTarget)
                {
                    ::SendMessage(hwndTarget, WM_COMMAND, LOWORD(pCI->nWindowsCmdID), (LPARAM) &cData);
                }
            }
            else
            {
                
                continue;
            }

            return S_OK;
        }

        
        if (!bCmdGroupFound)
        {
            OLECMDERR_E_UNKNOWNGROUP;
        }

        return OLECMDERR_E_NOTSUPPORTED;
    }
};




#define BEGIN_OLECOMMAND_TABLE() \
    OleCommandInfo *GetCommandTable() \
    { \
        static OleCommandInfo s_aSupportedCommands[] = \
        {

#define OLECOMMAND_MESSAGE(id, group, cmd, verb, desc) \
            { id, group, cmd, NULL, verb, desc },

#define OLECOMMAND_HANDLER(id, group, handler, verb, desc) \
            { id, group, 0, handler, verb, desc },

#define END_OLECOMMAND_TABLE() \
            { 0, &GUID_NULL, 0, NULL, NULL, NULL } \
        }; \
        return s_aSupportedCommands; \
    };

#endif
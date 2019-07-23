




































#define INCL_GPI
#define INCL_WIN
#include <os2.h>
#include <assert.h>
#include <stdio.h>

#include "npnulos2.h"

#include "plugin.h"
#include "utils.h"
#include "dialogs.h"
#include "dbg.h"
#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

nsIServiceManager * gServiceManager = NULL;

static char szNullPluginWindowClassName[] = CLASS_NULL_PLUGIN;

MRESULT EXPENTRY PluginWndProc(HWND, ULONG, MPARAM, MPARAM);

static char szDefaultPluginFinderURL[] = DEFAULT_PLUGINFINDER_URL;
static char szPageUrlForJavaScript[] = PAGE_URL_FOR_JAVASCRIPT;
static char szPageUrlForJVM[] = JVM_SMARTUPDATE_URL;

static char szPluginFinderCommandBeginning[] = PLUGINFINDER_COMMAND_BEGINNING;
static char szPluginFinderCommandEnd[] = PLUGINFINDER_COMMAND_END;

BOOL RegisterNullPluginWindowClass()
{
  return WinRegisterClass( (HAB)0, szNullPluginWindowClassName, (PFNWP)PluginWndProc, 0, sizeof(ULONG));
}

void UnregisterNullPluginWindowClass()
{
}







CPlugin::CPlugin(HMODULE hInst, 
                 NPP pNPInstance, 
                 SHORT wMode, 
                 NPMIMEType pluginType, 
                 PSZ szPageURL, 
                 PSZ szFileURL, 
                 PSZ szFileExtension,
                 BOOL bHidden) :
  m_hInst(hInst),
  m_pNPInstance(pNPInstance),
  m_wMode(wMode),
  m_hWnd(NULL),
  m_hWndParent(NULL),
  m_hIcon(NULL),
  m_szURLString(NULL),
  m_szCommandMessage(NULL),
  m_bWaitingStreamFromPFS(FALSE),
  m_PFSStream(NULL),
  m_bHidden(bHidden),
  m_pNPMIMEType(NULL),
  m_szPageURL(NULL),
  m_szFileURL(NULL),
  m_szFileExtension(NULL),
  m_hWndDialog(NULL),
  m_bOnline(TRUE),
  m_bJava(TRUE),
  m_bJavaScript(TRUE),
  m_bSmartUpdate(TRUE)
{
  dbgOut1("CPlugin::CPlugin()");
  assert(m_hInst != NULL);
  assert(m_pNPInstance != NULL);

  if(pluginType && *pluginType)
  {
    m_pNPMIMEType = (NPMIMEType)new char[strlen((PSZ)pluginType) + 1];
    if(m_pNPMIMEType != NULL)
      strcpy((PSZ)m_pNPMIMEType, pluginType);
  }

  if(szPageURL && *szPageURL)
  {
    m_szPageURL = new char[strlen(szPageURL) + 1];
    if(m_szPageURL != NULL)
      strcpy(m_szPageURL, szPageURL);
  }
  
  if(szFileURL && *szFileURL)
  {
    m_szFileURL = new char[strlen(szFileURL) + 1];
    if(m_szFileURL != NULL)
      strcpy(m_szFileURL, szFileURL);
  }

  if(szFileExtension && *szFileExtension)
  {
    m_szFileExtension = new char[strlen(szFileExtension) + 1];
    if(m_szFileExtension != NULL)
      strcpy(m_szFileExtension, szFileExtension);
  }

  m_hIcon = WinLoadPointer(HWND_DESKTOP, m_hInst, IDI_PLUGICON);

  char szString[1024] = {'\0'};
  WinLoadString((HAB)0, m_hInst, IDS_CLICK_TO_GET, sizeof(szString), szString);
  if(*szString)
  {
    m_szCommandMessage = new char[strlen(szString) + 1];
    if(m_szCommandMessage != NULL)
      strcpy(m_szCommandMessage, szString);
  }
}

CPlugin::~CPlugin()
{
  dbgOut1("CPlugin::~CPlugin()");

  if(m_pNPMIMEType != NULL)
  {
    delete [] m_pNPMIMEType;
    m_pNPMIMEType = NULL;
  }

  if(m_szPageURL != NULL)
  {
    delete [] m_szPageURL;
    m_szPageURL = NULL;
  }

  if(m_szFileURL != NULL)
  {
    delete [] m_szFileURL;
    m_szFileURL = NULL;
  }

  if(m_szFileExtension != NULL)
  {
    delete [] m_szFileExtension;
    m_szFileExtension = NULL;
  }

  if(m_szURLString != NULL)
  {
    delete [] m_szURLString;
    m_szURLString = NULL;
  }

  if(m_hIcon != NULL)
  {
    WinDestroyPointer(m_hIcon);
    m_hIcon = NULL;
  }

  if(m_szCommandMessage != NULL)
  {
    delete [] m_szCommandMessage;
    m_szCommandMessage = NULL;
  }
}

BOOL CPlugin::init(HWND hWndParent)
{
  dbgOut1("CPlugin::init()");

  nsISupports * sm = NULL;
  nsIPrefBranch * prefBranch = NULL;
  PRBool bSendUrls = PR_FALSE; 

  
  NPN_GetValue(NULL, NPNVserviceManager, &sm);

  
  if(sm) {
    sm->QueryInterface(NS_GET_IID(nsIServiceManager), (void**)&gServiceManager);
    NS_RELEASE(sm);
  }
  
  if (gServiceManager) {
    
    gServiceManager->GetServiceByContractID(NS_PREFSERVICE_CONTRACTID,
                                            NS_GET_IID(nsIPrefBranch),
                                            (void **)&prefBranch);
    if(prefBranch) {
      prefBranch->GetBoolPref("application.use_ns_plugin_finder", &bSendUrls);
      NS_RELEASE(prefBranch);
    }
  }
  m_bSmartUpdate = bSendUrls;

  if(!m_bHidden)
  {
    assert(WinIsWindow((HAB)0, hWndParent));

    if(WinIsWindow((HAB)0, hWndParent))
      m_hWndParent = hWndParent;

    RECTL rcParent;
    WinQueryWindowRect(m_hWndParent, &rcParent);

    m_hWnd = WinCreateWindow(m_hWndParent,
                             szNullPluginWindowClassName, 
                             "NULL Plugin", 
                             0,
                             0,0, rcParent.xRight, rcParent.yTop,
                             m_hWndParent,
                             HWND_TOP,
                             255,
                             (PVOID)this,
                             0);

    WinSetPresParam(m_hWnd, PP_FONTNAMESIZE, 10, (PVOID)"9.WarpSans");

    assert(m_hWnd != NULL);
    if((m_hWnd == NULL) || (!WinIsWindow((HAB)0, m_hWnd)))
      return FALSE;


    WinShowWindow(m_hWnd, TRUE);
  }

  if(IsNewMimeType((PSZ)m_pNPMIMEType) || m_bHidden)
    showGetPluginDialog();

  return TRUE;
}

void CPlugin::shut()
{
  dbgOut1("CPlugin::shut()");

  if(m_hWndDialog != NULL)
  {
    WinDismissDlg(m_hWndDialog, DID_CANCEL);
    m_hWndDialog = NULL;
  }

  if(m_hWnd != NULL)
  {
    WinDestroyWindow(m_hWnd);
    m_hWnd = NULL;
  }

  
  NS_IF_RELEASE(gServiceManager);
  gServiceManager = NULL;
}

HWND CPlugin::getWindow()
{
  return m_hWnd;
}

BOOL CPlugin::useDefaultURL_P()
{
  if((m_szPageURL == NULL) && (m_szFileURL == NULL))
    return TRUE;
  else
    return FALSE;
}

BOOL CPlugin::doSmartUpdate_P()
{
  
  return FALSE;

  if(m_bOnline && m_bJava && m_bJavaScript && m_bSmartUpdate && useDefaultURL_P())
    return TRUE;
  else
    return FALSE;
}

PSZ CPlugin::createURLString()
{
  if(m_szURLString != NULL)
  {
    delete [] m_szURLString;
    m_szURLString = NULL;
  }

  
  if(!m_bSmartUpdate && m_szFileURL != NULL)
  {
    m_szURLString = new char[strlen(m_szFileURL) + 1];
    if(m_szURLString == NULL)
      return NULL;
    strcpy(m_szURLString, m_szFileURL);
    return m_szURLString;
  }
  
  
  char * szAddress = NULL;
  char *urlToOpen = NULL;
  char contentTypeIsJava = 0;

  if (NULL != m_pNPMIMEType) {
    contentTypeIsJava = (0 == strcmp("application/x-java-vm",
				     m_pNPMIMEType)) ? 1 : 0;
  }
  
  if(!m_bSmartUpdate && m_szPageURL != NULL && !contentTypeIsJava)
  {
    szAddress = new char[strlen(m_szPageURL) + 1];
    if(szAddress == NULL)
      return NULL;
    strcpy(szAddress, m_szPageURL);

    m_szURLString = new char[strlen(szAddress) + 1 +
                             strlen((PSZ)m_pNPMIMEType) + 1];

    if(m_szURLString == NULL)
      return NULL;

    
    sprintf(m_szURLString, "%s?%s", szAddress, (PSZ)m_pNPMIMEType);
  }
  else 
  {
    if(!m_bSmartUpdate)
    {
      urlToOpen = szDefaultPluginFinderURL;
      
      if (contentTypeIsJava) {
        urlToOpen = szPageUrlForJVM;
      }

      szAddress = new char[strlen(urlToOpen) + 1];
      if(szAddress == NULL)
        return NULL;
      strcpy(szAddress, urlToOpen);

      m_szURLString = new char[strlen(szAddress) + 10 +
                               strlen((PSZ)m_pNPMIMEType) + 1];

      if(m_szURLString == NULL)
        return NULL;

      
      sprintf(m_szURLString, "%s?mimetype=%s",
              szAddress, (PSZ)m_pNPMIMEType);
    }
    else
    {
      urlToOpen = szPageUrlForJavaScript;

      if (contentTypeIsJava) {
	urlToOpen = szPageUrlForJVM;
      }

      
      
      if (!m_szPageURL) {
        m_szPageURL = new char[1];
        m_szPageURL[0] = '\0';
      }

      if (!m_szFileURL) {
        m_szFileURL = new char[1];
        m_szFileURL[0] = '\0';
      }

      m_szURLString = new char[strlen(szPluginFinderCommandBeginning) + strlen(urlToOpen) + 10 + 
                               strlen((PSZ)m_pNPMIMEType) + 13 +
                               strlen((PSZ)m_szPageURL) + 11 + 
                               strlen((PSZ)m_szFileURL) +
                               strlen(szPluginFinderCommandEnd) + 1];
      sprintf(m_szURLString, "%s%s?mimetype=%s&pluginspage=%s&pluginurl=%s%s",
              szPluginFinderCommandBeginning, urlToOpen, 
              (PSZ)m_pNPMIMEType, m_szPageURL, m_szFileURL, szPluginFinderCommandEnd);
    }
  }

  if(szAddress != NULL)
    delete [] szAddress;

  return m_szURLString;
}

void CPlugin::getPluginRegular()
{
  assert(m_bOnline);

  char * szURL = createURLString();

  assert(szURL != NULL);
  if(szURL == NULL)
    return;

  dbgOut3("CPlugin::getPluginRegular(), %#08x '%s'", m_pNPInstance, szURL);

  NPN_GetURL(m_pNPInstance, szURL, NULL);
  m_bWaitingStreamFromPFS = TRUE;
}

void CPlugin::getPluginSmart()
{















}

void CPlugin::showGetPluginDialog()
{
  assert(m_pNPMIMEType != NULL);
  if(m_pNPMIMEType == NULL)
    return;

  
  BOOL bOffline = FALSE;

  NPN_GetValue(m_pNPInstance, NPNVisOfflineBool, (void *)&bOffline);
  NPN_GetValue(m_pNPInstance, NPNVjavascriptEnabledBool, (void *)&m_bJavaScript);
  

  m_bOnline = !bOffline;

#ifdef OJI
  if(m_bOnline && m_bJavaScript && m_bSmartUpdate && useDefaultURL_P())
  {
    JRIEnv *penv = NPN_GetJavaEnv();
    m_bJava = (penv != NULL);
  }
#else
  m_bJava = FALSE;
#endif
  
  dbgOut1("Environment:");
  dbgOut2("%s", m_bOnline ? "On-line" : "Off-line");
  dbgOut2("Java %s", m_bJava ? "Enabled" : "Disabled");
  dbgOut2("JavaScript %s", m_bJavaScript ? "Enabled" : "Disabled");
  dbgOut2("SmartUpdate %s", m_bSmartUpdate ? "Enabled" : "Disabled");

  if((!m_bSmartUpdate && (m_szPageURL != NULL) || (m_szFileURL != NULL)) || !m_bJavaScript)
  {
    int iRet = WinDlgBox(HWND_DESKTOP, m_hWnd, (PFNWP)GetPluginDialogProc, m_hInst,
                                  IDD_PLUGIN_DOWNLOAD, (PVOID)this);
    if(iRet != IDC_GET_PLUGIN)
      return;
  }
  else
     getPlugin();
}

void CPlugin::getPlugin()
{
  if(m_szCommandMessage != NULL)
  {
    delete [] m_szCommandMessage;
    m_szCommandMessage = NULL;
  }

  char szString[1024] = {'\0'};
  WinLoadString((HAB)0, m_hInst, IDS_CLICK_WHEN_DONE, sizeof(szString), szString);
  if(*szString)
  {
    m_szCommandMessage = new char[strlen(szString) + 1];
    if(m_szCommandMessage != NULL)
      strcpy(m_szCommandMessage, szString);
  }

  WinInvalidateRect(m_hWnd, NULL, TRUE);


  getPluginRegular();
}




void CPlugin::resize()
{
  dbgOut1("CPlugin::resize()");
}

void CPlugin::print(NPPrint * pNPPrint)
{
  dbgOut1("CPlugin::print()");

  if(pNPPrint == NULL)
    return;
}

void CPlugin::URLNotify(const char * szURL)
{
  dbgOut2("CPlugin::URLNotify(), URL '%s'", szURL);

  NPStream * pStream = NULL;
  char buf[256];

  assert(m_hInst != NULL);
  assert(m_pNPInstance != NULL);
  
  int iSize = WinLoadString((HAB)0, m_hInst, IDS_GOING2HTML, sizeof(buf), buf);

  NPError rc = NPN_NewStream(m_pNPInstance, "text/html", "asd_plugin_finder", &pStream);
  if (rc != NPERR_NO_ERROR)
    return;

  
  
  NPN_Write(m_pNPInstance, pStream, iSize, buf);

  NPN_DestroyStream(m_pNPInstance, pStream, NPRES_DONE);
}

NPError CPlugin::newStream(NPMIMEType type, NPStream *stream, NPBool seekable, uint16 *stype)
{
  if (!m_bWaitingStreamFromPFS)
    return NPERR_NO_ERROR;

  m_bWaitingStreamFromPFS = FALSE;
  m_PFSStream = stream;

  if (stream) {
    if (type && !strcmp(type, "application/x-xpinstall"))
      NPN_GetURL(m_pNPInstance, stream->url, "_self");
    else
      NPN_GetURL(m_pNPInstance, stream->url, "_blank");
  }

  return NPERR_NO_ERROR;
}

NPError CPlugin::destroyStream(NPStream *stream, NPError reason)
{
  if (stream == m_PFSStream)
    m_PFSStream = NULL;

  return NPERR_NO_ERROR;
}

BOOL CPlugin::readyToRefresh()
{
  char szString[1024] = {'\0'};
  WinLoadString((HAB)0, m_hInst, IDS_CLICK_WHEN_DONE, sizeof(szString), szString);
  if(m_szCommandMessage == NULL)
    return FALSE;

  return (strcmp(m_szCommandMessage, szString) == 0);
}




void CPlugin::onCreate(HWND hWnd)
{
  m_hWnd = hWnd;
}

void CPlugin::onLButtonUp(HWND hWnd, int x, int y, UINT keyFlags)
{
  if(!readyToRefresh())
    showGetPluginDialog();
  else
    NPN_GetURL(m_pNPInstance, "javascript:navigator.plugins.refresh(true)", "_self");
}

void CPlugin::onRButtonUp(HWND hWnd, int x, int y, UINT keyFlags)
{
  if(!readyToRefresh())
    showGetPluginDialog();
  else
    NPN_GetURL(m_pNPInstance, "javascript:navigator.plugins.refresh(true)", "_self");
}

static void DrawCommandMessage(HPS hPS, PSZ szString, PRECTL lprc)
{
  if(szString == NULL)
    return;

  POINTL ptls[5];
  GpiQueryTextBox(hPS, strlen(szString), szString, 5, ptls);

  
  if (ptls[TXTBOX_CONCAT].x > lprc->xRight)
     return;

  RECTL rcText = rcText = *lprc; 

  
  
  rcText.yTop -= 80;

  WinDrawText(hPS, strlen(szString), szString, &rcText, 0, 0,
              DT_TEXTATTRS | DT_CENTER | DT_VCENTER);
}

#define INSET 1

void CPlugin::onPaint(HWND hWnd)
{
  RECTL rc;
  HDC hPS;
  int x, y;

  hPS = WinBeginPaint(hWnd, NULLHANDLE, NULL);
  GpiErase(hPS);
  WinQueryWindowRect(hWnd, &rc);

  x = (rc.xRight / 2) - (40 / 2);
  y = (rc.yTop / 2) - ((40) / 2);

  
  if(rc.xRight > (40 + 6 + INSET) && rc.yTop > (40 + 6 + INSET) )
  {
    if(m_hIcon != NULL)
      WinDrawPointer(hPS, x, y, m_hIcon, DP_NORMAL);
  }

  POINTL pt[5];

  
  GpiSetColor(hPS, CLR_WHITE);

  pt[0].x = 1 + INSET;
  pt[0].y = 1 + INSET;
  GpiMove(hPS, &pt[0]);

  pt[0].x = rc.xRight - 2 - INSET;
  pt[0].y = 1 + INSET;
  pt[1].x = rc.xRight - 2 - INSET;
  pt[1].y = rc.yTop -1 - INSET;

  GpiPolyLine(hPS, 2, pt);

  pt[0].x = 2 + INSET;
  pt[0].y = 3 + INSET;
  GpiMove(hPS, &pt[0]);

  pt[0].x = 2 + INSET;
  pt[0].y = rc.yTop - 3 - INSET;
  pt[1].x = rc.xRight - 4 - INSET;
  pt[1].y = rc.yTop - 3 - INSET;

  GpiPolyLine(hPS, 2, pt);

  
  GpiSetColor(hPS, CLR_PALEGRAY);
  pt[0].x = INSET;
  pt[0].y = 1 + INSET;
  GpiMove(hPS, &pt[0]);

  pt[0].x = INSET;
  pt[0].y = rc.yTop - 1 - INSET;
  pt[1].x = rc.xRight - 2 - INSET;
  pt[1].y = rc.yTop - 1 - INSET;

  GpiPolyLine(hPS, 2, pt);

  pt[0].x = rc.xRight - 3 - INSET;
  pt[0].y = rc.yTop - 2 - INSET;
  GpiMove(hPS, &pt[0]);

  pt[0].x = rc.xRight - 3 - INSET;
  pt[0].y = 2 + INSET;
  pt[1].x = 2 + INSET;
  pt[1].y = 2 + INSET;

  GpiPolyLine(hPS, 2, pt);

  
  GpiSetColor(hPS, CLR_DARKGRAY);

  pt[0].x = 1 + INSET;
  pt[0].y = 2 + INSET;
  GpiMove(hPS, &pt[0]);

  pt[0].x = 1 + INSET;
  pt[0].y = rc.yTop - 2 - INSET;
  pt[1].x = rc.xRight - 4 - INSET;
  pt[1].y = rc.yTop - 2 - INSET;

  GpiPolyLine(hPS, 2, pt);

  
  GpiSetColor(hPS, CLR_BLACK);

  pt[0].x = rc.xRight - 1 - INSET;
  pt[0].y = rc.yTop - 1 - INSET;
  GpiMove(hPS, &pt[0]);

  pt[0].x = rc.xRight - 1 - INSET;
  pt[0].y = 0 + INSET;
  pt[1].x = 0 + INSET;
  pt[1].y = 0 + INSET;

  GpiPolyLine(hPS, 2, pt);

  
  
  rc.xLeft += 4+INSET;
  rc.xRight -= 4+INSET;
  rc.yTop -= 4+INSET;
  rc.yBottom += 4+INSET;

  DrawCommandMessage(hPS, m_szCommandMessage, &rc);

  WinEndPaint (hPS);
}




MRESULT EXPENTRY NP_LOADDS PluginWndProc(HWND hWnd, ULONG message, MPARAM mp1, MPARAM mp2)
{
  CPlugin *pPlugin = (CPlugin *)WinQueryWindowULong(hWnd, QWL_USER);
                        
  switch(message)
  {
    case WM_CREATE:
      pPlugin = (CPlugin *)(((PCREATESTRUCT)mp2)->pCtlData);
      assert(pPlugin != NULL);
      WinSetWindowULong(hWnd, QWL_USER, (ULONG)pPlugin);
      pPlugin->onCreate(hWnd);
      return 0L;
    case WM_BUTTON1UP:
      pPlugin->onLButtonUp(hWnd,0,0,0);
      return 0L;
    case WM_BUTTON2UP:
      pPlugin->onRButtonUp(hWnd,0,0,0);
      return 0L;
    case WM_PAINT:
      pPlugin->onPaint(hWnd);
      return 0L;
    case WM_MOUSEMOVE:
      dbgOut1("MouseMove");
      break;

    default:
      break;
  }
  return(WinDefWindowProc(hWnd, message, mp1, mp2));
}

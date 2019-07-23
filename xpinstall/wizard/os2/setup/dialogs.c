







































#include "extern.h"
#include "extra.h"
#include "dialogs.h"
#include "ifuncns.h"
#include "xpistub.h"
#include "xpi.h"
#include "logging.h"
#include <logkeys.h>

static PFNWP OldListBoxWndProc;
static BOOL    gbProcessingXpnstallFiles;
static DWORD   gdwACFlag;
static DWORD   gdwIndexLastSelected;

BOOL AskCancelDlg(HWND hDlg)
{
  char szDlgQuitTitle[MAX_BUF];
  char szDlgQuitMsg[MAX_BUF];
  char szMsg[MAX_BUF];
  BOOL bRv = FALSE;

  if((sgProduct.ulMode != SILENT) && (sgProduct.ulMode != AUTO))
  {
    if(!GetPrivateProfileString("Messages", "DLGQUITTITLE", "", szDlgQuitTitle, sizeof(szDlgQuitTitle), szFileIniInstall))
      WinPostQueueMsg(0, WM_QUIT, 1, 0);
    else if(!GetPrivateProfileString("Messages", "DLGQUITMSG", "", szDlgQuitMsg, sizeof(szDlgQuitMsg), szFileIniInstall))
      WinPostQueueMsg(0, WM_QUIT, 1, 0);
    else if(WinMessageBox(HWND_DESKTOP, hDlg, szDlgQuitMsg, szDlgQuitTitle, 0, MB_YESNO | MB_ICONQUESTION | MB_MOVEABLE | MB_DEFBUTTON2 | MB_APPLMODAL) == MBID_YES)
    {
      WinDestroyWindow(hDlg);
      WinPostQueueMsg(0, WM_QUIT, 0, 0);
      bRv = TRUE;
    }
  }
  else
  {
    GetPrivateProfileString("Strings", "Message Cancel Setup AUTO mode", "", szMsg, sizeof(szMsg), szFileIniConfig);
    ShowMessage(szMsg, TRUE);
    DosSleep(5000);
    ShowMessage(szMsg, FALSE);
    bRv = TRUE;
  }

  return(bRv);
} 









void AdjustDialogSize(HWND hwndDlg)
{
  FONTMETRICS fm;
  HPS hps;
  float duX, duY;
  LONG cxDlgFrame, cyDlgFrame, cyTitleBar;
  SWP swpCurrent, swpDlg;
  HENUM henum;
  HWND hwndNext;
  USHORT id;
  CHAR classname[4];

  hps = WinGetPS(hwndDlg);
  GpiQueryFontMetrics(hps, sizeof(FONTMETRICS), &fm);
  WinReleasePS(hps);
  duX = fm.lAveCharWidth*0.25;
  duY = fm.lMaxBaselineExt*0.125;
  cxDlgFrame = WinQuerySysValue(HWND_DESKTOP, SV_CXDLGFRAME);
  cyDlgFrame = WinQuerySysValue(HWND_DESKTOP, SV_CYDLGFRAME);
  cyTitleBar = WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);

  WinQueryWindowPos(hwndDlg, &swpDlg);
  swpDlg.cx = ((swpDlg.cx-(cxDlgFrame*2))/duX)+(cxDlgFrame*2);
  swpDlg.cy = ((swpDlg.cy-cyTitleBar-(cyDlgFrame*2))/duY)+cyTitleBar+(cyDlgFrame*2);
  WinSetWindowPos(hwndDlg, 0, 0, 0,
                  swpDlg.cx,
                  swpDlg.cy,
                  SWP_SIZE);

  henum = WinBeginEnumWindows(hwndDlg);
  while ((hwndNext = WinGetNextWindow(henum)) != NULLHANDLE)
  {
    id = WinQueryWindowUShort( hwndNext, QWS_ID );
    if ((id == FID_TITLEBAR) ||
        (id == FID_SYSMENU) ||
        (id == FID_MINMAX) ||
        (id == FID_MENU))
    {
      continue;
    }
    WinQueryWindowPos(hwndNext, &swpCurrent);
    WinQueryClassName(hwndNext, 4, classname);
    if (strcmp(classname, "#6") == 0) {
      swpCurrent.x += 3;
      swpCurrent.y += 3;
      swpCurrent.cx -= 6;
      swpCurrent.cy -= 6;
      WinSetWindowPos(hwndNext, 0,
                      (swpCurrent.x-4)/duX+cyDlgFrame+3,
                      (swpCurrent.y-4)/duY+cyDlgFrame+3,
                      swpCurrent.cx/duX-6,
                      swpCurrent.cy/duY-6,
                      SWP_MOVE | SWP_SIZE);
    } else {
      WinSetWindowPos(hwndNext, 0,
                      (swpCurrent.x-4)/duX+cyDlgFrame,
                      (swpCurrent.y-4)/duY+cyDlgFrame,
                      swpCurrent.cx/duX,
                      swpCurrent.cy/duY,
                      SWP_MOVE | SWP_SIZE);
    }
  }
}

MRESULT EXPENTRY DlgProcWelcome(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  BOOL rc;
  char szBuf[MAX_BUF];
  SWP  swpDlg;
  
  switch(msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hDlg);
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      
      WinSetWindowText(hDlg, diWelcome.szTitle);
      sprintf(szBuf, diWelcome.szMessage0, sgProduct.szProductName, sgProduct.szProductName);
      strcat(szBuf, "\n\n");
      strcat(szBuf, diWelcome.szMessage1);
      strcat(szBuf, "\n\n");
      strcat(szBuf, diWelcome.szMessage2);
      WinSetDlgItemText(hDlg, IDC_STATIC0, szBuf);


      WinSetDlgItemText(hDlg, IDWIZNEXT, sgInstallGui.szNext_);
      WinSetDlgItemText(hDlg, IDCANCEL, sgInstallGui.szCancel_);

      
      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);
      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDWIZNEXT:
          WinDestroyWindow(hDlg);
          DlgSequence(NEXT_DLG);
          break;

        case DID_CANCEL:
          AskCancelDlg(hDlg);
          return (MRESULT)TRUE;
          break;

        default:
          break;
      }
      break;
  }
  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}

MRESULT EXPENTRY DlgProcLicense(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char            szBuf[MAX_BUF];
  LPSTR           szLicenseFilenameBuf = NULL;
  DWORD           dwFileSize;
  DWORD           dwBytesRead;
  HANDLE          hFLicense;
  FILE            *fLicense;
  SWP             swpDlg;

  switch(msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hDlg);
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      
      WinSetWindowText(hDlg, diLicense.szTitle);
      WinSetDlgItemText(hDlg, IDC_MESSAGE0, diLicense.szMessage0);
      WinSetDlgItemText(hDlg, IDC_MESSAGE1, diLicense.szMessage1);
      WinSetDlgItemText(hDlg, IDWIZBACK, sgInstallGui.szBack_);
      WinSetDlgItemText(hDlg, IDWIZNEXT, sgInstallGui.szAccept_);
      WinSetDlgItemText(hDlg, DID_CANCEL, sgInstallGui.szDecline_);

      
      strcpy(szBuf, szSetupDir);
      AppendBackSlash(szBuf, sizeof(szBuf));
      strcat(szBuf, diLicense.szLicenseFilename);

      if((fLicense = fopen(szBuf, "rb")) != NULL)
      {
        fseek(fLicense, 0L, SEEK_END);
        dwFileSize = ftell(fLicense);
        fseek(fLicense, 0L, SEEK_SET);
        if((szLicenseFilenameBuf = NS_GlobalAlloc(dwFileSize+1)) != NULL) {
          dwBytesRead = fread(szLicenseFilenameBuf, sizeof(char), dwFileSize, fLicense);
          szLicenseFilenameBuf[dwFileSize] = '\0';
          WinSetDlgItemText(hDlg, IDC_EDIT_LICENSE, szLicenseFilenameBuf);
          FreeMemory(&szLicenseFilenameBuf);
        }
        fclose(fLicense);
      }

      
      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);
      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDWIZNEXT:
          WinDestroyWindow(hDlg);
          DlgSequence(NEXT_DLG);
          break;

        case IDWIZBACK:
          WinDestroyWindow(hDlg);
          DlgSequence(PREV_DLG);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          return (MRESULT)TRUE;
          break;

        default:
          break;
      }
      break;
  }
  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}


MRESULT EXPENTRY DirDialogProc( HWND hwndDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   char szBuf[MAX_BUF];
   PFILEDLG pfiledlg;
   HRESULT hresult;

   switch ( msg ) {
      case WM_INITDLG:
         {
         SWP swpDlgOrig;
         SWP swpDlgNew;
         SWP swpDirST;
         SWP swpDirLB;
         SWP swpDriveST;
         SWP swpDriveCB;
         SWP swpDriveCBEF;
         SWP swpOK;
         SWP swpCancel;
         HWND hwndDirST;
         HWND hwndDirLB;
         HWND hwndDriveST;
         HWND hwndDriveCB;
         HWND hwndOK;
         HWND hwndCancel;
         HWND hwndEF;
         HENUM henum;
         HWND hwndNext;
         ULONG ulCurY, ulCurX;

         WinSetPresParam(hwndDlg, PP_FONTNAMESIZE,
                         strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
         hwndDirST = WinWindowFromID(hwndDlg, DID_DIRECTORY_TXT);
         hwndDirLB = WinWindowFromID(hwndDlg, DID_DIRECTORY_LB);
         hwndDriveST = WinWindowFromID(hwndDlg, DID_DRIVE_TXT);
         hwndDriveCB = WinWindowFromID(hwndDlg, DID_DRIVE_CB);
         hwndOK = WinWindowFromID(hwndDlg, DID_OK);
         hwndCancel = WinWindowFromID(hwndDlg, DID_CANCEL);
         
#define SPACING 10
         
         ulCurY = SPACING;
         ulCurX = SPACING + gSystemInfo.lDlgFrameX;
         WinQueryWindowPos(hwndOK, &swpOK);
         WinSetWindowPos(hwndOK, 0, ulCurX, ulCurY, 0, 0, SWP_MOVE);
         ulCurY += swpOK.cy + SPACING;
         WinQueryWindowPos(hwndCancel, &swpCancel);
         WinSetWindowPos(hwndCancel, 0, ulCurX+swpOK.cx+10, SPACING, 0, 0, SWP_MOVE);
         WinQueryWindowPos(hwndDirLB, &swpDirLB);
         WinSetWindowPos(hwndDirLB, 0, ulCurX, ulCurY, 0, 0, SWP_MOVE);
         ulCurY += swpDirLB.cy + SPACING;
         WinQueryWindowPos(hwndDirST, &swpDirST);
         WinSetWindowPos(hwndDirST, 0, ulCurX, ulCurY, 0, 0, SWP_MOVE);
         ulCurY += swpDirST.cy + SPACING;
         WinQueryWindowPos(hwndDriveCB, &swpDriveCB);
         WinQueryWindowPos(WinWindowFromID(hwndDriveCB, CBID_EDIT), &swpDriveCBEF);
         WinSetWindowPos(hwndDriveCB, 0, ulCurX, ulCurY-(swpDriveCB.cy-swpDriveCBEF.cy)+5,
                                         swpDirLB.cx,
                                         swpDriveCB.cy,
                                         SWP_SIZE | SWP_MOVE);
         ulCurY += swpDriveCBEF.cy + SPACING;
         WinQueryWindowPos(hwndDriveST, &swpDriveST);
         WinSetWindowPos(hwndDriveST, 0, ulCurX, ulCurY, 0, 0, SWP_MOVE);
         ulCurY += swpDriveST.cy + SPACING;
         
         hwndEF = WinCreateWindow(hwndDlg, WC_ENTRYFIELD, "", ES_MARGIN | WS_VISIBLE | ES_AUTOSCROLL,
                                  ulCurX+3, ulCurY+3,
                                  swpDirLB.cx-6, swpDriveCBEF.cy, hwndDlg, HWND_TOP, 777, NULL, NULL);
         WinSendMsg(hwndEF, EM_SETTEXTLIMIT, CCHMAXPATH, 0);
         ulCurY += swpDriveCBEF.cy + SPACING;

         
         henum = WinBeginEnumWindows(hwndDlg);
         while ((hwndNext = WinGetNextWindow(henum)) != NULLHANDLE)
         {
           USHORT usID = WinQueryWindowUShort(hwndNext, QWS_ID);
           if (usID != DID_DIRECTORY_TXT &&
               usID != DID_DIRECTORY_LB &&
               usID != DID_DRIVE_TXT &&
               usID != DID_DRIVE_CB &&
               usID != DID_OK &&
               usID != DID_CANCEL &&
               usID != FID_TITLEBAR &&
               usID != FID_SYSMENU &&
               usID != 777 &&
               usID != FID_MINMAX) 
           {
             WinShowWindow(hwndNext, FALSE);
           }
         }

         WinSetWindowPos(hwndDlg,
                         HWND_TOP,
                         (gSystemInfo.lScreenX/2)-((swpDirLB.cx+2*SPACING+2*gSystemInfo.lDlgFrameX)/2),
                         (gSystemInfo.lScreenY/2)-((ulCurY+2*gSystemInfo.lDlgFrameY+gSystemInfo.lTitleBarY)/2),
                         swpDirLB.cx+2*SPACING+2*gSystemInfo.lDlgFrameX,
                         ulCurY+2*gSystemInfo.lDlgFrameY+gSystemInfo.lTitleBarY,
                         SWP_MOVE | SWP_SIZE);
         }
         break;
      case WM_CONTROL:
         {
           if (SHORT1FROMMP(mp1) != 777) {
             pfiledlg = (PFILEDLG)WinQueryWindowPtr(hwndDlg, QWL_USER);
             RemoveBackSlash(pfiledlg->szFullFile);
             WinSetWindowText(WinWindowFromID(hwndDlg,777), pfiledlg->szFullFile);
           }
         }
         break;
      case WM_COMMAND:
        switch ( SHORT1FROMMP( mp1 ) ) {
        case DID_OK:
          WinQueryDlgItemText(hwndDlg, 777, MAX_BUF, szBuf);
          if(*szBuf != '\0')
             hresult = FileExists(szBuf);

          if ((*szBuf == '\0') || ((hresult == TRUE) && (hresult != FILE_DIRECTORY)))
          {
            char szEDestinationPath[MAX_BUF];

            GetPrivateProfileString("Messages", "ERROR_DESTINATION_PATH", "", szEDestinationPath, sizeof(szEDestinationPath), szFileIniInstall);
            WinMessageBox(HWND_DESKTOP, hwndDlg, szEDestinationPath, NULL, 0, MB_OK | MB_ICONEXCLAMATION);
            return (MRESULT)TRUE;
          }

          if (isFAT(szBuf)) {
            GetPrivateProfileString("Messages", "ERROR_FILESYSTEM", "", szBuf, sizeof(szBuf), szFileIniInstall);
            WinMessageBox(HWND_DESKTOP, hwndDlg, szBuf, NULL, 0, MB_OK | MB_ICONEXCLAMATION);
            return (MRESULT)TRUE;
          }

          if(hresult == FALSE)
          {
            char szMsgCreateDirectory[MAX_BUF];
            char szStrCreateDirectory[MAX_BUF];
            char szBufTemp[MAX_BUF];
            char szBufTemp2[MAX_BUF];

            GetPrivateProfileString("Messages", "STR_CREATE_DIRECTORY", "", szStrCreateDirectory, sizeof(szStrCreateDirectory), szFileIniInstall);
            if(GetPrivateProfileString("Messages", "MSG_CREATE_DIRECTORY", "", szMsgCreateDirectory, sizeof(szMsgCreateDirectory), szFileIniInstall))
            {
              strcpy(szBufTemp, "\n\n");
              strcat(szBufTemp, szBuf);
              RemoveBackSlash(szBufTemp);
              strcat(szBufTemp, "\n\n");
              sprintf(szBufTemp2, szMsgCreateDirectory, szBufTemp);
            }

            if(WinMessageBox(HWND_DESKTOP, hwndDlg, szBufTemp2, szStrCreateDirectory, 0, MB_YESNO | MB_ICONQUESTION) == IDYES)
            {
              char szBuf2[MAX_PATH];

              AppendBackSlash(szBuf, sizeof(szBuf));
              if(CreateDirectoriesAll(szBuf, TRUE) == FALSE)
              {
                char szECreateDirectory[MAX_BUF];

                strcpy(szBufTemp, "\n\n");
                strcat(szBufTemp, szBuf);
                RemoveBackSlash(szBufTemp);
                strcat(szBufTemp, "\n\n");

                if(GetPrivateProfileString("Messages", "ERROR_CREATE_DIRECTORY", "", szECreateDirectory, sizeof(szECreateDirectory), szFileIniInstall))
                  sprintf(szBuf, szECreateDirectory, szBufTemp);

                WinMessageBox(HWND_DESKTOP, hwndDlg, szBuf, "", 0, MB_OK | MB_ERROR);
                return (MRESULT)TRUE;
              }
            } else {
             return (MRESULT)TRUE;
            }
          }
          RemoveBackSlash(szBuf);
          strcpy(szTempSetupPath, szBuf);
          break;


        }
       break;
   }      
   return WinDefFileDlgProc(hwndDlg, msg, mp1, mp2);
}

BOOL BrowseForDirectory(HWND hDlg, char *szCurrDir)
{ 
  FILEDLG        filedlg;
  char           ftitle[MAX_PATH];
  char           szBuf[MAX_BUF];
  char           szSearchPathBuf[MAX_BUF];
  char           szDlgBrowseTitle[MAX_BUF];
  BOOL           bRet;
  char*          caret;

  memset(szDlgBrowseTitle, 0, sizeof(szDlgBrowseTitle));
  GetPrivateProfileString("Messages", "DLGBROWSETITLE", "", szDlgBrowseTitle, sizeof(szDlgBrowseTitle), szFileIniInstall);

  memset(&filedlg, 0, sizeof(FILEDLG));
  filedlg.cbSize = sizeof(FILEDLG);
  filedlg.pszTitle = szDlgBrowseTitle;
  strcpy(filedlg.szFullFile, szCurrDir);
  strncat(filedlg.szFullFile, "\\", 1);
  strncat(filedlg.szFullFile, "^", 1);
  filedlg.fl = FDS_OPEN_DIALOG | FDS_CENTER;
  filedlg.pfnDlgProc = DirDialogProc;
  WinFileDlg(HWND_DESKTOP, hDlg, &filedlg);
  caret = strstr(filedlg.szFullFile, "^");
  if (caret) {
    *(caret-1) = '\0';
  } else {
    
    
    
  }

  if (filedlg.lReturn == DID_OK) {
    bRet = TRUE;
  } else {
    strcpy(szTempSetupPath, szCurrDir);
    bRet = FALSE;
  }

  return(bRet);
}

void TruncateString(HWND hWnd, LPSTR szInURL, LPSTR szOutString, DWORD dwOutStringBufSize)
{
  HPS           hpsWnd;
  SWP           swpWnd;
  RECTL         rectlString = {0,0,1000,1000};
  char          *ptr = NULL;
  int           iHalfLen;
  int           iOutStringLen;

  if((DWORD)strlen(szInURL) > dwOutStringBufSize)
    return;

  memset(szOutString, 0, dwOutStringBufSize);
  strcpy(szOutString, szInURL);
  iOutStringLen = strlen(szOutString);

  hpsWnd = WinGetPS(hWnd);


  WinQueryWindowPos(hWnd, &swpWnd);

  WinDrawText(hpsWnd, iOutStringLen, szOutString,
                              &rectlString, 0, 0, 
                              DT_BOTTOM | DT_QUERYEXTENT | DT_TEXTATTRS);
  while(rectlString.xRight > swpWnd.cx)
  {
    iHalfLen = iOutStringLen / 2;
    if(iHalfLen == 2)
      break;

    ptr = szOutString + iHalfLen;
    memmove(ptr - 1, ptr, strlen(ptr) + 1);
    szOutString[iHalfLen - 2] = '.';
    szOutString[iHalfLen - 1] = '.';
    szOutString[iHalfLen]     = '.';
    iOutStringLen = strlen(szOutString);
    rectlString.xLeft = rectlString.yBottom = 0;
    rectlString.xRight = rectlString.yTop = 1000;
    WinDrawText(hpsWnd, iOutStringLen, szOutString,
                &rectlString, 0, 0, 
                DT_BOTTOM | DT_QUERYEXTENT | DT_TEXTATTRS);
  }

  WinReleasePS(hpsWnd);
}


MRESULT EXPENTRY DlgProcSetupType(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  HWND          hRadioSt0;
  HWND          hStaticSt0;
  HWND          hRadioSt1;
  HWND          hStaticSt1;
  HWND          hRadioSt2;
  HWND          hStaticSt2;
  HWND          hRadioSt3;
  HWND          hStaticSt3;
  HWND          hReadme;
  HWND          hDestinationPath;
  SWP           swpDlg;
  char          szBuf[MAX_BUF];
  char          szBufTemp[MAX_BUF];
  char          szBufTemp2[MAX_BUF];
  HRESULT       hresult;
  HOBJECT       hobject;

  hRadioSt0   = WinWindowFromID(hDlg, IDC_RADIO_ST0);
  hStaticSt0  = WinWindowFromID(hDlg, IDC_STATIC_ST0_DESCRIPTION);
  hRadioSt1   = WinWindowFromID(hDlg, IDC_RADIO_ST1);
  hStaticSt1  = WinWindowFromID(hDlg, IDC_STATIC_ST1_DESCRIPTION);
  hRadioSt2   = WinWindowFromID(hDlg, IDC_RADIO_ST2);
  hStaticSt2  = WinWindowFromID(hDlg, IDC_STATIC_ST2_DESCRIPTION);
  hRadioSt3   = WinWindowFromID(hDlg, IDC_RADIO_ST3);
  hStaticSt3  = WinWindowFromID(hDlg, IDC_STATIC_ST3_DESCRIPTION);
  hReadme     = WinWindowFromID(hDlg, IDC_README);

  switch(msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hDlg);
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      WinSetWindowText(hDlg, diSetupType.szTitle);

      hDestinationPath = WinWindowFromID(hDlg, IDC_EDIT_DESTINATION); 
      strcpy(szTempSetupPath, sgProduct.szPath);
      TruncateString(hDestinationPath, szTempSetupPath, szBuf, sizeof(szBuf));

      WinSetDlgItemText(hDlg, IDC_EDIT_DESTINATION, szBuf);
      WinSetDlgItemText(hDlg, IDC_STATIC_MSG0, diSetupType.szMessage0);

      if(diSetupType.stSetupType0.bVisible)
      {
        WinSetDlgItemText(hDlg, IDC_RADIO_ST0, diSetupType.stSetupType0.szDescriptionShort);
        WinSetDlgItemText(hDlg, IDC_STATIC_ST0_DESCRIPTION, diSetupType.stSetupType0.szDescriptionLong);
        WinShowWindow(hRadioSt0, TRUE);
        WinShowWindow(hStaticSt0, TRUE);
      }
      else
      {
        WinShowWindow(hRadioSt0, FALSE);
        WinShowWindow(hStaticSt0, FALSE);
      }

      if(diSetupType.stSetupType1.bVisible)
      {
        WinSetDlgItemText(hDlg, IDC_RADIO_ST1, diSetupType.stSetupType1.szDescriptionShort);
        WinSetDlgItemText(hDlg, IDC_STATIC_ST1_DESCRIPTION, diSetupType.stSetupType1.szDescriptionLong);
        WinShowWindow(hRadioSt1, TRUE);
        WinShowWindow(hStaticSt1, TRUE);
      }
      else
      {
        WinShowWindow(hRadioSt1, FALSE);
        WinShowWindow(hStaticSt1, FALSE);
      }

      if(diSetupType.stSetupType2.bVisible)
      {
        WinSetDlgItemText(hDlg, IDC_RADIO_ST2, diSetupType.stSetupType2.szDescriptionShort);
        WinSetDlgItemText(hDlg, IDC_STATIC_ST2_DESCRIPTION, diSetupType.stSetupType2.szDescriptionLong);
        WinShowWindow(hRadioSt2, TRUE);
        WinShowWindow(hStaticSt2, TRUE);
      }
      else
      {
        WinShowWindow(hRadioSt2, FALSE);
        WinShowWindow(hStaticSt2, FALSE);
      }

      if(diSetupType.stSetupType3.bVisible)
      {
        WinSetDlgItemText(hDlg, IDC_RADIO_ST3, diSetupType.stSetupType3.szDescriptionShort);
        WinSetDlgItemText(hDlg, IDC_STATIC_ST3_DESCRIPTION, diSetupType.stSetupType3.szDescriptionLong);
        WinShowWindow(hRadioSt3, TRUE);
        WinShowWindow(hStaticSt3, TRUE);
      }
      else
      {
        WinShowWindow(hRadioSt3, FALSE);
        WinShowWindow(hStaticSt3, FALSE);
      }

      
      switch(ulTempSetupType)
      {
        case ST_RADIO0:
          WinCheckButton(hDlg, IDC_RADIO_ST0, 1);
          WinSetFocus(HWND_DESKTOP, hRadioSt0);
          break;

        case ST_RADIO1:
          WinCheckButton(hDlg, IDC_RADIO_ST1, 1);
          WinSetFocus(HWND_DESKTOP, hRadioSt1);
          break;

        case ST_RADIO2:
          WinCheckButton(hDlg, IDC_RADIO_ST2, 1);
          WinSetFocus(HWND_DESKTOP, hRadioSt2);
          break;

        case ST_RADIO3:
          WinCheckButton(hDlg, IDC_RADIO_ST3, 1);
          WinSetFocus(HWND_DESKTOP, hRadioSt3);
          break;
      }

      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);

      strcpy(szBuf, szSetupDir);
      AppendBackSlash(szBuf, sizeof(szBuf));
      strcat(szBuf, diSetupType.szReadmeFilename);
      if((*diSetupType.szReadmeFilename == '\0') || (FileExists(szBuf) == FALSE))
        WinShowWindow(hReadme, FALSE);
      else
        WinShowWindow(hReadme, TRUE);

      WinSetDlgItemText(hDlg, IDC_DESTINATION, sgInstallGui.szDestinationDirectory);
      WinSetDlgItemText(hDlg, IDC_BUTTON_BROWSE, sgInstallGui.szBrowse_);
      WinSetDlgItemText(hDlg, IDWIZBACK, sgInstallGui.szBack_);
      WinSetDlgItemText(hDlg, IDWIZNEXT, sgInstallGui.szNext_);
      WinSetDlgItemText(hDlg, IDCANCEL, sgInstallGui.szCancel_);
      WinSetDlgItemText(hDlg, IDC_README, sgInstallGui.szReadme_);

      if(sgProduct.bLockPath)
        WinEnableWindow(WinWindowFromID(hDlg, IDC_BUTTON_BROWSE), FALSE);
      else
        WinEnableWindow(WinWindowFromID(hDlg, IDC_BUTTON_BROWSE), TRUE);

      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDC_BUTTON_BROWSE:
          if(WinQueryButtonCheckstate(hDlg, IDC_RADIO_ST0)      == 1)
            ulTempSetupType = ST_RADIO0;
          else if(WinQueryButtonCheckstate(hDlg, IDC_RADIO_ST1) == 1)
            ulTempSetupType = ST_RADIO1;
          else if(WinQueryButtonCheckstate(hDlg, IDC_RADIO_ST2) == 1)
            ulTempSetupType = ST_RADIO2;
          else if(WinQueryButtonCheckstate(hDlg, IDC_RADIO_ST3) == 1)
            ulTempSetupType = ST_RADIO3;

          BrowseForDirectory(hDlg, szTempSetupPath);

          hDestinationPath = WinWindowFromID(hDlg, IDC_EDIT_DESTINATION); 
          TruncateString(hDestinationPath, szTempSetupPath, szBuf, sizeof(szBuf));
          WinSetDlgItemText(hDlg, IDC_EDIT_DESTINATION, szBuf);
          return (MRESULT)TRUE;
          break;

        case IDC_README:
          {
          char fullReadme[CCHMAXPATH];
          strcpy(fullReadme, szSetupDir);
          strcat(fullReadme, diSetupType.szReadmeFilename);

          if(*diSetupType.szReadmeApp == '\0')
            WinSpawn(diSetupType.szReadmeFilename, NULL, szSetupDir, FALSE);
          else
            WinSpawn(diSetupType.szReadmeApp, fullReadme, szSetupDir, FALSE);
          }
          return (MRESULT)TRUE;
          break;

        case IDWIZNEXT:
          strcpy(sgProduct.szPath, szTempSetupPath);

          strcpy(szBuf, sgProduct.szPath);

          if (isFAT(szBuf)) {
            GetPrivateProfileString("Messages", "ERROR_FILESYSTEM", "", szBuf, sizeof(szBuf), szFileIniInstall);
            WinMessageBox(HWND_DESKTOP, hDlg, szBuf, NULL, 0, MB_OK | MB_ICONEXCLAMATION);
            return (MRESULT)TRUE;
          }

          hresult = FileExists(szBuf);
          if ((hresult == TRUE) && (hresult != FILE_DIRECTORY))
          {
            char szEDestinationPath[MAX_BUF];

            GetPrivateProfileString("Messages", "ERROR_DESTINATION_PATH", "", szEDestinationPath, sizeof(szEDestinationPath), szFileIniInstall);
            WinMessageBox(HWND_DESKTOP, hDlg, szEDestinationPath, NULL, 0, MB_OK | MB_ICONEXCLAMATION);
            return (MRESULT)TRUE;
          }

          if(FileExists(szBuf) == FALSE)
          {
            char szMsgCreateDirectory[MAX_BUF];
            char szStrCreateDirectory[MAX_BUF];

            GetPrivateProfileString("Messages", "STR_CREATE_DIRECTORY", "", szStrCreateDirectory, sizeof(szStrCreateDirectory), szFileIniInstall);
            if(GetPrivateProfileString("Messages", "MSG_CREATE_DIRECTORY", "", szMsgCreateDirectory, sizeof(szMsgCreateDirectory), szFileIniInstall))
            {
              strcpy(szBufTemp, "\n\n");
              strcat(szBufTemp, szBuf);
              RemoveBackSlash(szBufTemp);
              strcat(szBufTemp, "\n\n");
              sprintf(szBufTemp2, szMsgCreateDirectory, szBufTemp);
            }

            if(WinMessageBox(HWND_DESKTOP, hDlg, szBufTemp2, szStrCreateDirectory, 0, MB_YESNO | MB_ICONQUESTION) == MBID_YES)
            {
              char szBuf2[MAX_PATH];

              

              AppendBackSlash(szBuf, sizeof(szBuf));
              if(CreateDirectoriesAll(szBuf, TRUE) == FALSE)
              {
                char szECreateDirectory[MAX_BUF];

                strcpy(szBufTemp, "\n\n");
                strcat(szBufTemp, sgProduct.szPath);
                RemoveBackSlash(szBufTemp);
                strcat(szBufTemp, "\n\n");

                if(GetPrivateProfileString("Messages", "ERROR_CREATE_DIRECTORY", "", szECreateDirectory, sizeof(szECreateDirectory), szFileIniInstall))
                  sprintf(szBuf, szECreateDirectory, szBufTemp);

                WinMessageBox(HWND_DESKTOP, hDlg, szBuf, "", 0, MB_OK | MB_ERROR);
                return (MRESULT)TRUE;
              }

              if(*sgProduct.szSubPath != '\0')
              {
                 


                strcpy(szBuf2, szBuf);
                AppendBackSlash(szBuf2, sizeof(szBuf2));
                strcat(szBuf2, sgProduct.szSubPath);
                UpdateInstallLog(KEY_CREATE_FOLDER, szBuf2, FALSE);
              }

              bCreateDestinationDir = TRUE;
            }
            else
            {
              return (MRESULT)TRUE;
            }
          }

          
          if(WinQueryButtonCheckstate(hDlg, IDC_RADIO_ST0)      == 1)
          {
            SiCNodeSetItemsSelected(ST_RADIO0);

            ulSetupType     = ST_RADIO0;
            ulTempSetupType = ulSetupType;
          }
          else if(WinQueryButtonCheckstate(hDlg, IDC_RADIO_ST1) == 1)
          {
            SiCNodeSetItemsSelected(ST_RADIO1);

            ulSetupType     = ST_RADIO1;
            ulTempSetupType = ulSetupType;
          }
          else if(WinQueryButtonCheckstate(hDlg, IDC_RADIO_ST2) == 1)
          {
            SiCNodeSetItemsSelected(ST_RADIO2);

            ulSetupType     = ST_RADIO2;
            ulTempSetupType = ulSetupType;
          }
          else if(WinQueryButtonCheckstate(hDlg, IDC_RADIO_ST3) == 1)
          {
            SiCNodeSetItemsSelected(ST_RADIO3);

            ulSetupType     = ST_RADIO3;
            ulTempSetupType = ulSetupType;
          }

          WinDestroyWindow(hDlg);
          DlgSequence(NEXT_DLG);
          break;

        case IDWIZBACK:
          ulTempSetupType = ulSetupType;
          strcpy(sgProduct.szPath, szTempSetupPath);
          WinDestroyWindow(hDlg);
          DlgSequence(PREV_DLG);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          return (MRESULT)TRUE;
          break;

        default:
          break;
      }
      break;
  }
  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}

void DrawLBText(POWNERITEM poi, DWORD dwACFlag)
{
  siC     *siCTemp  = NULL;
  CHAR   chBuffer[MAX_BUF];
  DWORD      y;

  siCTemp = SiCNodeGetObject(poi->idItem, FALSE, dwACFlag);
  if(siCTemp != NULL)
  {
    WinSendMsg(poi->hwnd, LM_QUERYITEMTEXT, MPFROM2SHORT(poi->idItem, MAX_BUF), (MPARAM)chBuffer);

    if (poi->fsState != poi->fsStateOld) {
      if (!(siCTemp->dwAttributes & SIC_DISABLED))
        WinInvertRect(poi->hps, &poi->rclItem);
    } else {
       ULONG flags = DT_TEXTATTRS | DT_LEFT | DT_VCENTER;
       RECTL rclTemp = poi->rclItem;
       rclTemp.xLeft += CX_CHECKBOX+4+4;
       if(siCTemp->dwAttributes & SIC_DISABLED)
         flags |= DT_HALFTONE;
       WinFillRect(poi->hps, &rclTemp, CLR_BACKGROUND);
       WinDrawText(poi->hps, -1, chBuffer, &rclTemp, 0, 0,
                   flags);
       if (poi->fsState) {
         if (!(siCTemp->dwAttributes & SIC_DISABLED))
           WinInvertRect(poi->hps, &poi->rclItem);
       }
    }
    poi->fsState = poi->fsStateOld = 0;
  }
}

void DrawCheck(POWNERITEM poi, DWORD dwACFlag)
{
  siC     *siCTemp  = NULL;
  HBITMAP hbmpCheckBox;
  POINTL ptl;

  siCTemp = SiCNodeGetObject(poi->idItem, FALSE, dwACFlag);
  if(siCTemp != NULL)
  {
    if(!(siCTemp->dwAttributes & SIC_SELECTED))
      


      hbmpCheckBox = GpiLoadBitmap(poi->hps, hSetupRscInst, IDB_BOX_UNCHECKED, CX_CHECKBOX, CY_CHECKBOX);
    else if(siCTemp->dwAttributes & SIC_DISABLED)
      
      hbmpCheckBox = GpiLoadBitmap(poi->hps, hSetupRscInst, IDB_BOX_CHECKED_DISABLED, CX_CHECKBOX, CY_CHECKBOX);
    else
      
      hbmpCheckBox = GpiLoadBitmap(poi->hps, hSetupRscInst, IDB_BOX_CHECKED, CX_CHECKBOX, CY_CHECKBOX);

    ptl.x = poi->rclItem.xLeft+4;
    ptl.y = poi->rclItem.yBottom+4; 
    WinDrawBitmap(poi->hps, hbmpCheckBox, NULL, &ptl, 0, 0, DBM_NORMAL);
    GpiDeleteBitmap(hbmpCheckBox);
  }
}

void InvalidateLBCheckbox(HWND hwndListBox)
{
  RECTL rcCheckArea;

  
  WinQueryWindowRect(hwndListBox, &rcCheckArea);

  
  
  
  
  if(!gSystemInfo.bScreenReader)
    rcCheckArea.xRight = CX_CHECKBOX;

  
  
  
  
  
  WinInvalidateRect(hwndListBox, &rcCheckArea, TRUE);
}
  
void ToggleCheck(HWND hwndListBox, DWORD dwIndex, DWORD dwACFlag)
{
  BOOL  bMoreToResolve;
  LPSTR szToggledReferenceName = NULL;
  DWORD dwAttributes;
  CHAR tchBuffer[MAX_BUF];

  
  
  dwAttributes = SiCNodeGetAttributes(dwIndex, FALSE, dwACFlag);
  if(!(dwAttributes & SIC_DISABLED))
  {
    if(dwAttributes & SIC_SELECTED)
    {
      SiCNodeSetAttributes(dwIndex, SIC_SELECTED, FALSE, FALSE, dwACFlag, hwndListBox);

      szToggledReferenceName = SiCNodeGetReferenceName(dwIndex, FALSE, dwACFlag);
      ResolveDependees(szToggledReferenceName, hwndListBox);
    }
    else
    {
      SiCNodeSetAttributes(dwIndex, SIC_SELECTED, TRUE, FALSE, dwACFlag, hwndListBox);
      bMoreToResolve = ResolveDependencies(dwIndex, hwndListBox);

      while(bMoreToResolve)
        bMoreToResolve = ResolveDependencies(-1, hwndListBox);

      szToggledReferenceName = SiCNodeGetReferenceName(dwIndex, FALSE, dwACFlag);
      ResolveDependees(szToggledReferenceName, hwndListBox);
    }
    InvalidateLBCheckbox(hwndListBox);
  }
}







MRESULT APIENTRY NewListBoxWndProc(HWND hWnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  POINTS ptspointerpos;
  USHORT fsflags;
  ULONG ulIndex;
  
  switch(msg)
  {
    case WM_CHAR:
      fsflags = SHORT1FROMMP(mp1);
      if (SHORT2FROMMP(mp2) == VK_SPACE) {
        if (!(fsflags & KC_KEYUP)) {
          ulIndex = WinSendMsg(hWnd,
                               LM_QUERYSELECTION,
                               0,
                               0);
          ToggleCheck(hWnd, ulIndex, gdwACFlag);
        }
      }
      break;

    case WM_BUTTON1DOWN:
      ptspointerpos.x = SHORT1FROMMP(mp1);
      ptspointerpos.y = SHORT2FROMMP(mp1);

      if((ptspointerpos.x >= 0) && (ptspointerpos.x <= CX_CHECKBOX+4+4))
      {
        MRESULT mr = (OldListBoxWndProc)(hWnd, msg, mp1, mp2);
        ulIndex = WinSendMsg(hWnd, LM_QUERYSELECTION, LIT_FIRST, 0);
        ToggleCheck(hWnd, ulIndex, gdwACFlag);
        return mr;
      }
      break;
  }

  return(OldListBoxWndProc)(hWnd, msg, mp1, mp2);
}

MRESULT EXPENTRY DlgProcSelectComponents(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  BOOL                bReturn = FALSE;
  siC                 *siCTemp;
  DWORD               dwIndex;
  DWORD               dwItems = MAX_BUF;
  HWND                hwndLBComponents;
  SWP                 swpDlg;
  CHAR                tchBuffer[MAX_BUF];
  POWNERITEM          pOwnerItem;
  ULONG               ulDSBuf;
  char                szBuf[MAX_BUF];

  hwndLBComponents  = WinWindowFromID(hDlg, IDC_LIST_COMPONENTS);

  switch(msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hDlg);
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      WinSetWindowText(hDlg, diSelectComponents.szTitle);
      WinSetDlgItemText(hDlg, IDC_MESSAGE0, diSelectComponents.szMessage0);

      siCTemp = siComponents;
      if(siCTemp != NULL)
      {
        if((!(siCTemp->dwAttributes & SIC_INVISIBLE)) && (!(siCTemp->dwAttributes & SIC_ADDITIONAL)))
          WinSendMsg(hwndLBComponents, LM_INSERTITEM, LIT_END, (MPARAM)siCTemp->szDescriptionShort);

        siCTemp = siCTemp->Next;
        while((siCTemp != siComponents) && (siCTemp != NULL))
        {
          if((!(siCTemp->dwAttributes & SIC_INVISIBLE)) && (!(siCTemp->dwAttributes & SIC_ADDITIONAL)))
            WinSendMsg(hwndLBComponents, LM_INSERTITEM, LIT_END, (MPARAM)siCTemp->szDescriptionShort);

          siCTemp = siCTemp->Next;
        }
        WinSetFocus(HWND_DESKTOP, hwndLBComponents);
        WinSendMsg(hwndLBComponents, LM_SELECTITEM, 0, 0);
        WinSetDlgItemText(hDlg, IDC_STATIC_DESCRIPTION, SiCNodeGetDescriptionLong(0, FALSE, AC_COMPONENTS));
      }

      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);

      

      ulDSBuf = GetDiskSpaceAvailable(sgProduct.szPath);
      _itoa(ulDSBuf, tchBuffer, 10);
      ParsePath(sgProduct.szPath, szBuf, sizeof(szBuf), FALSE, PP_ROOT_ONLY);
      RemoveBackSlash(szBuf);
      strcat(szBuf, "   ");
      strcat(szBuf, tchBuffer);
      strcat(szBuf, " KB");
      WinSetDlgItemText(hDlg, IDC_SPACE_AVAILABLE, szBuf);

      WinSetDlgItemText(hDlg, IDC_STATIC1, sgInstallGui.szComponents_);
      WinSetDlgItemText(hDlg, IDC_STATIC2, sgInstallGui.szDescription);
      WinSetDlgItemText(hDlg, IDC_STATIC3, sgInstallGui.szTotalDownloadSize);
      WinSetDlgItemText(hDlg, IDC_STATIC4, sgInstallGui.szSpaceAvailable);
      WinSetDlgItemText(hDlg, IDWIZBACK, sgInstallGui.szBack_);
      WinSetDlgItemText(hDlg, IDWIZNEXT, sgInstallGui.szNext_);
      WinSetDlgItemText(hDlg, IDCANCEL, sgInstallGui.szCancel_);

      gdwACFlag = AC_COMPONENTS;
      OldListBoxWndProc = WinSubclassWindow(hwndLBComponents, (PFNWP)NewListBoxWndProc);
      break;

    case WM_MEASUREITEM:
      return MRFROM2SHORT(CX_CHECKBOX+4+4, 0);

    case WM_DRAWITEM:
      pOwnerItem = (POWNERITEM)mp2;

      
      if(pOwnerItem->idItem == -1)
        break;

      DrawLBText(pOwnerItem, AC_COMPONENTS);
      DrawCheck(pOwnerItem, AC_COMPONENTS);

      

      ulDSBuf = GetDiskSpaceRequired(DSR_DOWNLOAD_SIZE);
      _itoa(ulDSBuf, tchBuffer, 10);
      strcpy(szBuf, tchBuffer);
      strcat(szBuf, " KB");
      
      WinSetDlgItemText(hDlg, IDC_DOWNLOAD_SIZE, szBuf);
      return (MRESULT)TRUE;
      break;

    case WM_CONTROL:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDC_LIST_COMPONENTS:
          
          if((dwIndex = WinSendMsg(hwndLBComponents, LM_QUERYSELECTION, 0, 0)) != LIT_NONE)
            WinSetDlgItemText(hDlg, IDC_STATIC_DESCRIPTION, SiCNodeGetDescriptionLong(dwIndex, FALSE, AC_COMPONENTS));
          break;
      }
      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDWIZNEXT:
          WinDestroyWindow(hDlg);
          DlgSequence(NEXT_DLG);
          break;

        case IDWIZBACK:
          WinDestroyWindow(hDlg);
          DlgSequence(PREV_DLG);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          return (MRESULT)TRUE;
          break;

        default:
          break;
      }
      break;
  }

  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}

MRESULT EXPENTRY DlgProcSelectAdditionalComponents(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  BOOL                bReturn = FALSE;
  siC                 *siCTemp;
  DWORD               dwIndex;
  DWORD               dwItems = MAX_BUF;
  HWND                hwndLBComponents;
  SWP                 swpDlg;
  CHAR                tchBuffer[MAX_BUF];
  POWNERITEM          pOwnerItem;
  ULONG               ulDSBuf;
  char                szBuf[MAX_BUF];

  hwndLBComponents  = WinWindowFromID(hDlg, IDC_LIST_COMPONENTS);

  switch(msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hDlg);
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      WinSetWindowText(hDlg, diSelectAdditionalComponents.szTitle);
      WinSetDlgItemText(hDlg, IDC_MESSAGE0, diSelectAdditionalComponents.szMessage0);

      siCTemp = siComponents;
      if(siCTemp != NULL)
      {
        if((!(siCTemp->dwAttributes & SIC_INVISIBLE)) && (siCTemp->dwAttributes & SIC_ADDITIONAL))
          WinSendMsg(hwndLBComponents, LM_INSERTITEM, LIT_END, (MPARAM)siCTemp->szDescriptionShort);

        siCTemp = siCTemp->Next;
        while((siCTemp != siComponents) && (siCTemp != NULL))
        {
          if((!(siCTemp->dwAttributes & SIC_INVISIBLE)) && (siCTemp->dwAttributes & SIC_ADDITIONAL))
            WinSendMsg(hwndLBComponents, LM_INSERTITEM, LIT_END, (MPARAM)siCTemp->szDescriptionShort);

          siCTemp = siCTemp->Next;
        }
        WinSetFocus(HWND_DESKTOP, hwndLBComponents);
        WinSendMsg(hwndLBComponents, LM_SELECTITEM, 0, 0);
        WinSetDlgItemText(hDlg, IDC_STATIC_DESCRIPTION, SiCNodeGetDescriptionLong(0, FALSE, AC_ADDITIONAL_COMPONENTS));
      }

      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);

      

      ulDSBuf = GetDiskSpaceAvailable(sgProduct.szPath);
      _itoa(ulDSBuf, tchBuffer, 10);
      ParsePath(sgProduct.szPath, szBuf, sizeof(szBuf), FALSE, PP_ROOT_ONLY);
      RemoveBackSlash(szBuf);
      strcat(szBuf, "   ");
      strcat(szBuf, tchBuffer);
      strcat(szBuf, " KB");
      WinSetDlgItemText(hDlg, IDC_SPACE_AVAILABLE, szBuf);

      WinSetDlgItemText(hDlg, IDC_STATIC1, sgInstallGui.szAdditionalComponents_);
      WinSetDlgItemText(hDlg, IDC_STATIC2, sgInstallGui.szDescription);
      WinSetDlgItemText(hDlg, IDC_STATIC3, sgInstallGui.szTotalDownloadSize);
      WinSetDlgItemText(hDlg, IDC_STATIC4, sgInstallGui.szSpaceAvailable);
      WinSetDlgItemText(hDlg, IDWIZBACK, sgInstallGui.szBack_);
      WinSetDlgItemText(hDlg, IDWIZNEXT, sgInstallGui.szNext_);
      WinSetDlgItemText(hDlg, IDCANCEL, sgInstallGui.szCancel_);

      gdwACFlag = AC_ADDITIONAL_COMPONENTS;
      OldListBoxWndProc = WinSubclassWindow(hwndLBComponents, (PFNWP)NewListBoxWndProc);
      break;

    case WM_MEASUREITEM:
      return MRFROM2SHORT(CX_CHECKBOX+4+4, 0);

    case WM_DRAWITEM:
      pOwnerItem = (POWNERITEM)mp2;

      
      if(pOwnerItem->idItem == -1)
        break;

      DrawLBText(pOwnerItem, AC_ADDITIONAL_COMPONENTS);
      DrawCheck(pOwnerItem, AC_ADDITIONAL_COMPONENTS);

      

      ulDSBuf = GetDiskSpaceRequired(DSR_DOWNLOAD_SIZE);
      _itoa(ulDSBuf, tchBuffer, 10);
      strcpy(szBuf, tchBuffer);
      strcat(szBuf, " KB");
      
      WinSetDlgItemText(hDlg, IDC_DOWNLOAD_SIZE, szBuf);
      return (MRESULT)TRUE;
      break;

    case WM_CONTROL:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDC_LIST_COMPONENTS:
          
          if((dwIndex = WinSendMsg(hwndLBComponents, LM_QUERYSELECTION, 0, 0)) != LIT_NONE)
            WinSetDlgItemText(hDlg, IDC_STATIC_DESCRIPTION, SiCNodeGetDescriptionLong(dwIndex, FALSE, AC_ADDITIONAL_COMPONENTS));
          break;
      }
      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDWIZNEXT:
          WinDestroyWindow(hDlg);
          DlgSequence(NEXT_DLG);
          break;

        case IDWIZBACK:
          WinDestroyWindow(hDlg);
          DlgSequence(PREV_DLG);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          return (MRESULT)TRUE;
          break;

        default:
          break;
      }
      break;
  }

  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}

MRESULT APIENTRY DlgProcOS2Integration(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  char szBuf[MAX_BUF];
  HWND hcbCheck0;
  HWND hcbCheck1;
  HWND hcbCheck2;
  HWND hcbCheck3;
  HWND hDestinationPath;
  SWP                 swpDlg;

  hcbCheck0 = WinWindowFromID(hDlg, IDC_CHECK0);
  hcbCheck1 = WinWindowFromID(hDlg, IDC_CHECK1);
  hcbCheck2 = WinWindowFromID(hDlg, IDC_CHECK2);
  hcbCheck3 = WinWindowFromID(hDlg, IDC_CHECK3);

  switch(msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hDlg);
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      WinSetWindowText(hDlg, diOS2Integration.szTitle);

      if (diOS2Integration.szHomeDirectory[0]) {
        strcpy(szTempSetupPath, diOS2Integration.szHomeDirectory);
        hDestinationPath = WinWindowFromID(hDlg, IDC_EDIT_DESTINATION); 
        TruncateString(hDestinationPath, szTempSetupPath, szBuf, sizeof(szBuf));
      } else {
        strcpy(szTempSetupPath, sgProduct.szPath);
        hDestinationPath = WinWindowFromID(hDlg, IDC_EDIT_DESTINATION); 
        TruncateString(hDestinationPath, szTempSetupPath, szBuf, sizeof(szBuf));
      }

      WinSetDlgItemText(hDlg, IDC_EDIT_DESTINATION, szBuf);

      WinSetDlgItemText(hDlg, IDC_MESSAGE0, diOS2Integration.szMessage0);
      WinSetDlgItemText(hDlg, IDC_MESSAGE1, diOS2Integration.szMessage1);

      if(diOS2Integration.oiCBMakeDefaultBrowser.bEnabled)
      {
        WinShowWindow(hcbCheck0, TRUE);
        WinCheckButton(hDlg, IDC_CHECK0, diOS2Integration.oiCBMakeDefaultBrowser.bCheckBoxState);
        WinSetDlgItemText(hDlg, IDC_CHECK0, diOS2Integration.oiCBMakeDefaultBrowser.szDescription);
      }
      else
        WinShowWindow(hcbCheck0, FALSE);

      if(diOS2Integration.oiCBAssociateHTML.bEnabled)
      {
        WinShowWindow(hcbCheck1, TRUE);
        WinCheckButton(hDlg, IDC_CHECK1, diOS2Integration.oiCBAssociateHTML.bCheckBoxState);
        WinSetDlgItemText(hDlg, IDC_CHECK1, diOS2Integration.oiCBAssociateHTML.szDescription);
      }
      else
        WinShowWindow(hcbCheck1, FALSE);

      if(diOS2Integration.oiCBUpdateCONFIGSYS.bEnabled)
      {
        WinShowWindow(hcbCheck2, TRUE);
        WinCheckButton(hDlg, IDC_CHECK2, diOS2Integration.oiCBUpdateCONFIGSYS.bCheckBoxState);
        WinSetDlgItemText(hDlg, IDC_CHECK2, diOS2Integration.oiCBUpdateCONFIGSYS.szDescription);
      }
      else
        WinShowWindow(hcbCheck2, FALSE);

      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);

      WinSetDlgItemText(hDlg, IDC_BUTTON_BROWSE, sgInstallGui.szBrowse_);
      WinSetDlgItemText(hDlg, IDWIZBACK, sgInstallGui.szBack_);
      WinSetDlgItemText(hDlg, IDWIZNEXT, sgInstallGui.szNext_);
      WinSetDlgItemText(hDlg, IDCANCEL, sgInstallGui.szCancel_);
      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDC_BUTTON_BROWSE:
          if (BrowseForDirectory(hDlg, szTempSetupPath)) {
            strcpy(diOS2Integration.szHomeDirectory, szTempSetupPath);
          }

          hDestinationPath = WinWindowFromID(hDlg, IDC_EDIT_DESTINATION); 
          TruncateString(hDestinationPath, szTempSetupPath, szBuf, sizeof(szBuf));
          WinSetDlgItemText(hDlg, IDC_EDIT_DESTINATION, szBuf);
          return (MRESULT)TRUE;
          break;
        case IDWIZNEXT:
          if(diOS2Integration.oiCBMakeDefaultBrowser.bEnabled)
          {
            if(WinQueryButtonCheckstate(hDlg, IDC_CHECK0) == 1)
              diOS2Integration.oiCBMakeDefaultBrowser.bCheckBoxState = TRUE;
            else
              diOS2Integration.oiCBMakeDefaultBrowser.bCheckBoxState = FALSE;
          }

          if(diOS2Integration.oiCBAssociateHTML.bEnabled)
          {
            if(WinQueryButtonCheckstate(hDlg, IDC_CHECK1) == 1)
              diOS2Integration.oiCBAssociateHTML.bCheckBoxState = TRUE;
            else
              diOS2Integration.oiCBAssociateHTML.bCheckBoxState = FALSE;
          }

          if(diOS2Integration.oiCBUpdateCONFIGSYS.bEnabled)
          {
            if(WinQueryButtonCheckstate(hDlg, IDC_CHECK2) == 1)
              diOS2Integration.oiCBUpdateCONFIGSYS.bCheckBoxState = TRUE;
            else
              diOS2Integration.oiCBUpdateCONFIGSYS.bCheckBoxState = FALSE;
          }

          WinDestroyWindow(hDlg);
          DlgSequence(NEXT_DLG);
          break;

        case IDWIZBACK:
          WinDestroyWindow(hDlg);
          DlgSequence(PREV_DLG);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          return (MRESULT)TRUE;
          break;

        default:
          break;
      }
      break;
  }
  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}

void SaveDownloadProtocolOption(HWND hDlg)
{
  if(WinQueryButtonCheckstate(hDlg, IDC_USE_FTP) == 1)
    diAdditionalOptions.dwUseProtocol = UP_FTP;
  else if(WinQueryButtonCheckstate(hDlg, IDC_USE_HTTP) == 1)
    diAdditionalOptions.dwUseProtocol = UP_HTTP;
}

MRESULT EXPENTRY DlgProcAdvancedSettings(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  SWP   swpDlg;
  char  szBuf[MAX_BUF];

  switch(msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hDlg);
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      WinSetWindowText(hDlg, diAdvancedSettings.szTitle);
      WinSetDlgItemText(hDlg, IDC_MESSAGE0,          diAdvancedSettings.szMessage0);
      WinSetDlgItemText(hDlg, IDC_EDIT_PROXY_SERVER, diAdvancedSettings.szProxyServer);
      WinSetDlgItemText(hDlg, IDC_EDIT_PROXY_PORT,   diAdvancedSettings.szProxyPort);
      WinSetDlgItemText(hDlg, IDC_EDIT_PROXY_USER,   diAdvancedSettings.szProxyUser);
      WinSetDlgItemText(hDlg, IDC_EDIT_PROXY_PASSWD, diAdvancedSettings.szProxyPasswd);

      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);

      GetPrivateProfileString("Strings", "IDC Use FTP", "", szBuf, sizeof(szBuf), szFileIniConfig);
      WinSetDlgItemText(hDlg, IDC_USE_FTP, szBuf);
      GetPrivateProfileString("Strings", "IDC Use HTTP", "", szBuf, sizeof(szBuf), szFileIniConfig);
      WinSetDlgItemText(hDlg, IDC_USE_HTTP, szBuf);

      WinSetDlgItemText(hDlg, IDC_STATIC, sgInstallGui.szProxySettings);
      WinSetDlgItemText(hDlg, IDC_STATIC1, sgInstallGui.szServer);
      WinSetDlgItemText(hDlg, IDC_STATIC2, sgInstallGui.szPort);
      WinSetDlgItemText(hDlg, IDC_STATIC3, sgInstallGui.szUserId);
      WinSetDlgItemText(hDlg, IDC_STATIC4, sgInstallGui.szPassword);
      WinSetDlgItemText(hDlg, IDWIZNEXT, sgInstallGui.szOk_);
      WinSetDlgItemText(hDlg, IDCANCEL, sgInstallGui.szCancel_);

      switch(diAdditionalOptions.dwUseProtocol)
      {
        case UP_HTTP:
          WinCheckButton(hDlg, IDC_USE_FTP,  0);
          WinCheckButton(hDlg, IDC_USE_HTTP, 1);
          break;

        case UP_FTP:
        default:
          WinCheckButton(hDlg, IDC_USE_FTP,  1);
          WinCheckButton(hDlg, IDC_USE_HTTP, 0);
          break;

      }

      if((diAdditionalOptions.bShowProtocols) && (diAdditionalOptions.bUseProtocolSettings))
      {
        WinShowWindow(WinWindowFromID(hDlg, IDC_USE_FTP),  TRUE);
        WinShowWindow(WinWindowFromID(hDlg, IDC_USE_HTTP), TRUE);
      }
      else
      {
        WinShowWindow(WinWindowFromID(hDlg, IDC_USE_FTP),  FALSE);
        WinShowWindow(WinWindowFromID(hDlg, IDC_USE_HTTP), FALSE);
      }

      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDWIZNEXT:
          
          WinQueryDlgItemText(hDlg, IDC_EDIT_PROXY_SERVER, MAX_BUF, diAdvancedSettings.szProxyServer);
          WinQueryDlgItemText(hDlg, IDC_EDIT_PROXY_PORT,   MAX_BUF, diAdvancedSettings.szProxyPort);
          WinQueryDlgItemText(hDlg, IDC_EDIT_PROXY_USER,   MAX_BUF, diAdvancedSettings.szProxyUser);
          WinQueryDlgItemText(hDlg, IDC_EDIT_PROXY_PASSWD, MAX_BUF, diAdvancedSettings.szProxyPasswd);

          SaveDownloadProtocolOption(hDlg);
          WinDestroyWindow(hDlg);
          DlgSequence(NEXT_DLG);
          break;

        case IDWIZBACK:
        case IDCANCEL:
          WinDestroyWindow(hDlg);
          DlgSequence(PREV_DLG);
          break;

        default:
          break;
      }
      break;
  }
  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}

void SaveAdditionalOptions(HWND hDlg, HWND hwndCBSiteSelector)
{
  int iIndex;

  
  iIndex = WinSendMsg(hwndCBSiteSelector, LM_QUERYSELECTION, 0, 0);
  WinSendMsg(hwndCBSiteSelector, LM_QUERYITEMTEXT, MPFROM2SHORT(iIndex, MAX_BUF), (MPARAM)szSiteSelectorDescription);

  
  if(WinQueryButtonCheckstate(hDlg, IDC_CHECK_SAVE_INSTALLER_FILES) == 1)
    diAdditionalOptions.bSaveInstaller = TRUE;
  else
    diAdditionalOptions.bSaveInstaller = FALSE;

  
  if(WinQueryButtonCheckstate(hDlg, IDC_CHECK_RECAPTURE_HOMEPAGE) == 1)
    diAdditionalOptions.bRecaptureHomepage = TRUE;
  else
    diAdditionalOptions.bRecaptureHomepage = FALSE;
}

MRESULT EXPENTRY DlgProcAdditionalOptions(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  SWP   swpDlg;
  char  szBuf[MAX_BUF];
  HWND  hwndCBSiteSelector;
  int   iIndex;
  ssi   *ssiTemp;
  char  szCBDefault[MAX_BUF];

  hwndCBSiteSelector = WinWindowFromID(hDlg, IDC_LIST_SITE_SELECTOR);

  switch(msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hDlg);
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);






      if(diAdditionalOptions.bShowHomepageOption == FALSE)
      {
        WinShowWindow(WinWindowFromID(hDlg, IDC_MESSAGE0),  FALSE);
        WinShowWindow(WinWindowFromID(hDlg, IDC_CHECK_RECAPTURE_HOMEPAGE),  FALSE);
      }

      if(GetTotalArchivesToDownload() == 0)
     {
        WinShowWindow(WinWindowFromID(hDlg, IDC_MESSAGE1),  FALSE);
        WinShowWindow(WinWindowFromID(hDlg, IDC_CHECK_SAVE_INSTALLER_FILES),  FALSE);
        WinShowWindow(WinWindowFromID(hDlg, IDC_EDIT_LOCAL_INSTALLER_PATH), FALSE);
        WinShowWindow(WinWindowFromID(hDlg, IDC_BUTTON_PROXY_SETTINGS), FALSE);
      }

      WinSetWindowText(hDlg, diAdditionalOptions.szTitle);
      WinSetDlgItemText(hDlg, IDC_MESSAGE0, diAdditionalOptions.szMessage0);
      WinSetDlgItemText(hDlg, IDC_MESSAGE1, diAdditionalOptions.szMessage1);

      GetPrivateProfileString("Strings", "IDC Save Installer Files", "", szBuf, sizeof(szBuf), szFileIniConfig);
      WinSetDlgItemText(hDlg, IDC_CHECK_SAVE_INSTALLER_FILES, szBuf);
      GetPrivateProfileString("Strings", "IDC Recapture Homepage", "", szBuf, sizeof(szBuf), szFileIniConfig);
      WinSetDlgItemText(hDlg, IDC_CHECK_RECAPTURE_HOMEPAGE, szBuf);

      GetSaveInstallerPath(szBuf, sizeof(szBuf));
      WinSetDlgItemText(hDlg, IDC_EDIT_LOCAL_INSTALLER_PATH, szBuf);

      WinSetDlgItemText(hDlg, IDC_BUTTON_PROXY_SETTINGS, sgInstallGui.szProxySettings_);
      WinSetDlgItemText(hDlg, IDWIZBACK, sgInstallGui.szBack_);
      WinSetDlgItemText(hDlg, IDWIZNEXT, sgInstallGui.szNext_);
      WinSetDlgItemText(hDlg, IDCANCEL, sgInstallGui.szCancel_);

      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);

      ssiTemp = ssiSiteSelector;
      do
      {
        if(ssiTemp == NULL)
          break;

        WinSendMsg(hwndCBSiteSelector, LM_INSERTITEM, 0, (LPARAM)(ssiTemp->szDescription));
        ssiTemp = ssiTemp->Next;
      } while(ssiTemp != ssiSiteSelector);

      if((szSiteSelectorDescription == NULL) || (*szSiteSelectorDescription == '\0'))
      {
          if(GetPrivateProfileString("Messages", "CB_DEFAULT", "", szCBDefault, sizeof(szCBDefault), szFileIniInstall) &&
          ((iIndex = WinSendMsg(hwndCBSiteSelector, LM_SEARCHSTRING, MPFROM2SHORT(0, LIT_FIRST), (LPARAM)szCBDefault)) != LIT_NONE))
          WinSendMsg(hwndCBSiteSelector, LM_SELECTITEM, (WPARAM)iIndex, 0);
        else
          WinSendMsg(hwndCBSiteSelector, LM_SELECTITEM, 0, 0);
      }
      else if((iIndex = WinSendMsg(hwndCBSiteSelector, LM_SEARCHSTRING, MPFROM2SHORT(0, LIT_FIRST), (LPARAM)szSiteSelectorDescription)) != LIT_NONE)
        WinSendMsg(hwndCBSiteSelector, LM_SELECTITEM, (WPARAM)iIndex, 0);
      else
        WinSendMsg(hwndCBSiteSelector, LM_SELECTITEM, 0, 0);

      if(diAdditionalOptions.bSaveInstaller)
        WinCheckButton(hDlg, IDC_CHECK_SAVE_INSTALLER_FILES, 1);
      else
        WinCheckButton(hDlg, IDC_CHECK_SAVE_INSTALLER_FILES, 0);

      if(diAdditionalOptions.bRecaptureHomepage)
        WinCheckButton(hDlg, IDC_CHECK_RECAPTURE_HOMEPAGE, 1);
      else
        WinCheckButton(hDlg, IDC_CHECK_RECAPTURE_HOMEPAGE, 0);

      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDWIZNEXT:
          SaveAdditionalOptions(hDlg, hwndCBSiteSelector);
          WinDestroyWindow(hDlg);
          DlgSequence(NEXT_DLG);
          break;

        case IDWIZBACK:
          SaveAdditionalOptions(hDlg, hwndCBSiteSelector);
          WinDestroyWindow(hDlg);
          DlgSequence(PREV_DLG);
          break;

        case IDC_BUTTON_ADDITIONAL_SETTINGS:
          SaveAdditionalOptions(hDlg, hwndCBSiteSelector);
          WinDestroyWindow(hDlg);
          DlgSequence(OTHER_DLG_1);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          return (MRESULT)TRUE;
          break;

        default:
          break;
      }
      break;
  }
  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}

void AppendStringWOTilde(LPSTR szInputString, DWORD dwInputStringSize, LPSTR szString)
{
  DWORD i;
  DWORD iInputStringCounter;
  DWORD iInputStringLen;
  DWORD iStringLen;


  iInputStringLen = strlen(szInputString);
  iStringLen      = strlen(szString);

  if((iInputStringLen + iStringLen) >= dwInputStringSize)
    return;

  iInputStringCounter = iInputStringLen;
  for(i = 0; i < iStringLen; i++)
  {
    if(szString[i] != '~')
      szInputString[iInputStringCounter++] = szString[i];
    else
      if (szString[i-1] == '(') {
         szInputString[iInputStringCounter-1] = '\0';
         break;
      }
  }
}

LPSTR GetStartInstallMessage()
{
  char  szBuf[MAX_BUF];
  char  szSTRRequired[MAX_BUF_TINY];
  siC   *siCObject   = NULL;
  LPSTR szMessageBuf = NULL;
  DWORD dwBufSize;
  DWORD dwIndex0;

  GetPrivateProfileString("Strings", "STR Force Upgrade Required", "", szSTRRequired, sizeof(szSTRRequired), szFileIniConfig);

  
  dwBufSize = 0;

  
  if(GetPrivateProfileString("Messages", "STR_SETUP_TYPE", "", szBuf, sizeof(szBuf), szFileIniInstall))
    dwBufSize += strlen(szBuf) + 2; 
  dwBufSize += 4; 

  switch(ulSetupType)
  {
    case ST_RADIO3:
      dwBufSize += strlen(diSetupType.stSetupType3.szDescriptionShort) + 2; 
      break;

    case ST_RADIO2:
      dwBufSize += strlen(diSetupType.stSetupType2.szDescriptionShort) + 2; 
      break;

    case ST_RADIO1:
      dwBufSize += strlen(diSetupType.stSetupType1.szDescriptionShort) + 2; 
      break;

    default:
      dwBufSize += strlen(diSetupType.stSetupType0.szDescriptionShort) + 2; 
      break;
  }
  dwBufSize += 2; 

  
  if(GetPrivateProfileString("Messages", "STR_SELECTED_COMPONENTS", "", szBuf, sizeof(szBuf), szFileIniInstall))
    dwBufSize += strlen(szBuf) + 2; 

  dwIndex0 = 0;
  siCObject = SiCNodeGetObject(dwIndex0, FALSE, AC_ALL);
  while(siCObject)
  {
    if(siCObject->dwAttributes & SIC_SELECTED)
    {
      dwBufSize += 4; 
      dwBufSize += strlen(siCObject->szDescriptionShort);
    }

    if(siCObject->bForceUpgrade)
    {
      
      if(*szSTRRequired != '\0')
      {
        dwBufSize += 1; 
        dwBufSize += strlen(szSTRRequired);
      }
    }

    if(siCObject->dwAttributes & SIC_SELECTED)
      dwBufSize += 2; 

    ++dwIndex0;
    siCObject = SiCNodeGetObject(dwIndex0, FALSE, AC_ALL);
  }
  dwBufSize += 2; 

  
  if(GetPrivateProfileString("Messages", "STR_DESTINATION_DIRECTORY", "", szBuf, sizeof(szBuf), szFileIniInstall))
    dwBufSize += strlen(szBuf) + 2; 

  dwBufSize += 4; 
  dwBufSize += strlen(sgProduct.szPath) + 2; 
  dwBufSize += 2; 

  if(GetTotalArchivesToDownload() > 0)
  {
    dwBufSize += 2; 

    
    if(GetPrivateProfileString("Messages", "STR_DOWNLOAD_SITE", "", szBuf, sizeof(szBuf), szFileIniInstall))
      dwBufSize += strlen(szBuf) + 2; 

    dwBufSize += 4; 
    dwBufSize += strlen(szSiteSelectorDescription) + 2; 

    if(diAdditionalOptions.bSaveInstaller)
    {
      dwBufSize += 2; 

      
      if(GetPrivateProfileString("Messages", "STR_SAVE_INSTALLER_FILES", "", szBuf, sizeof(szBuf), szFileIniInstall))
        dwBufSize += strlen(szBuf) + 2; 

      GetSaveInstallerPath(szBuf, sizeof(szBuf));
      dwBufSize += 4; 
      dwBufSize += strlen(szBuf) + 2; 
    }
  }

  dwBufSize += 1; 


  



  
  if((szMessageBuf = NS_GlobalAlloc(dwBufSize)) != NULL)
  {
    memset(szMessageBuf, 0, dwBufSize);

    
    if(GetPrivateProfileString("Messages", "STR_SETUP_TYPE", "", szBuf, sizeof(szBuf), szFileIniInstall))
    {
      strcat(szMessageBuf, szBuf);
      strcat(szMessageBuf, "\r\n");
    }
    strcat(szMessageBuf, "    "); 
      
    switch(ulSetupType)
    {
      case ST_RADIO3:
        AppendStringWOTilde(szMessageBuf, dwBufSize, diSetupType.stSetupType3.szDescriptionShort);
        break;

      case ST_RADIO2:
        AppendStringWOTilde(szMessageBuf, dwBufSize, diSetupType.stSetupType2.szDescriptionShort);
        break;

      case ST_RADIO1:
        AppendStringWOTilde(szMessageBuf, dwBufSize, diSetupType.stSetupType1.szDescriptionShort);
        break;

      default:
        AppendStringWOTilde(szMessageBuf, dwBufSize, diSetupType.stSetupType0.szDescriptionShort);
        break;
    }
    strcat(szMessageBuf, "\r\n\r\n");

    
    if(GetPrivateProfileString("Messages", "STR_SELECTED_COMPONENTS", "", szBuf, sizeof(szBuf), szFileIniInstall))
    {
      strcat(szMessageBuf, szBuf);
      strcat(szMessageBuf, "\r\n");
    }

    dwIndex0  = 0;
    siCObject = SiCNodeGetObject(dwIndex0, FALSE, AC_ALL);
    while(siCObject)
    {
      if(siCObject->dwAttributes & SIC_SELECTED)
      {
        strcat(szMessageBuf, "    "); 
        strcat(szMessageBuf, siCObject->szDescriptionShort);
      }

      if(siCObject->bForceUpgrade)
      {
        
        if(*szSTRRequired != '\0')
        {
          strcat(szMessageBuf, " "); 
          strcat(szMessageBuf, szSTRRequired);
        }
      }

      if(siCObject->dwAttributes & SIC_SELECTED)
        strcat(szMessageBuf, "\r\n");

      ++dwIndex0;
      siCObject = SiCNodeGetObject(dwIndex0, FALSE, AC_ALL);
    }
    strcat(szMessageBuf, "\r\n");

    
    if(GetPrivateProfileString("Messages", "STR_DESTINATION_DIRECTORY", "", szBuf, sizeof(szBuf), szFileIniInstall))
    {
      strcat(szMessageBuf, szBuf);
      strcat(szMessageBuf, "\r\n");
    }
    strcat(szMessageBuf, "    "); 
    strcat(szMessageBuf, sgProduct.szPath);
    strcat(szMessageBuf, "\r\n\r\n");

    if(GetTotalArchivesToDownload() > 0)
    {
      strcat(szMessageBuf, "\r\n");

      
      if(GetPrivateProfileString("Messages", "STR_DOWNLOAD_SITE", "", szBuf, sizeof(szBuf), szFileIniInstall))
      {
        strcat(szMessageBuf, szBuf);
        strcat(szMessageBuf, "\r\n");
      }

      strcat(szMessageBuf, "    "); 
      strcat(szMessageBuf, szSiteSelectorDescription); 
      strcat(szMessageBuf, "\r\n");

      if(diAdditionalOptions.bSaveInstaller)
      {
        strcat(szMessageBuf, "\r\n");

        
        if(GetPrivateProfileString("Messages", "STR_SAVE_INSTALLER_FILES", "", szBuf, sizeof(szBuf), szFileIniInstall))
        {
          strcat(szMessageBuf, szBuf);
          strcat(szMessageBuf, "\r\n");
        }

        GetSaveInstallerPath(szBuf, sizeof(szBuf));
        strcat(szMessageBuf, "    "); 
        strcat(szMessageBuf, szBuf);
        strcat(szMessageBuf, "\r\n");
      }
    }
  }

  return(szMessageBuf);
}

MRESULT EXPENTRY DlgProcStartInstall(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  SWP   swpDlg;
  LPSTR szMessage = NULL;

  switch(msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hDlg);
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      WinSetWindowText(hDlg, diStartInstall.szTitle);

      WinSetDlgItemText(hDlg, IDC_STATIC, sgInstallGui.szCurrentSettings);
      WinSetDlgItemText(hDlg, IDWIZBACK, sgInstallGui.szBack_);
      WinSetDlgItemText(hDlg, IDWIZNEXT, sgInstallGui.szInstall_);
      WinSetDlgItemText(hDlg, IDCANCEL, sgInstallGui.szCancel_);
 
      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);

      if((diAdvancedSettings.bShowDialog == FALSE) || (GetTotalArchivesToDownload() == 0))
      {
        WinShowWindow(WinWindowFromID(hDlg, IDC_BUTTON_SITE_SELECTOR), FALSE);
        WinSetDlgItemText(hDlg, IDC_MESSAGE0, diStartInstall.szMessageInstall);
      }
      else
      {
        WinShowWindow(WinWindowFromID(hDlg, IDC_BUTTON_SITE_SELECTOR), TRUE);
        WinSetDlgItemText(hDlg, IDC_MESSAGE0, diStartInstall.szMessageDownload);
      }

      if((szMessage = GetStartInstallMessage()) != NULL)
      {
        WinSetDlgItemText(hDlg, IDC_CURRENT_SETTINGS, szMessage);
        FreeMemory(&szMessage);
      }

      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDWIZNEXT:
          WinDestroyWindow(hDlg);
          DlgSequence(NEXT_DLG);
          break;

        case IDWIZBACK:
          WinDestroyWindow(hDlg);
          DlgSequence(PREV_DLG);
          break;

        case IDCANCEL:
          AskCancelDlg(hDlg);
          return (MRESULT)TRUE;
          break;

        default:
          break;
      }
      break;
  }
  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}

MRESULT EXPENTRY DlgProcReboot(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  HANDLE            hToken;
  HWND              hRadioYes;
  SWP               swpDlg;

  hRadioYes = WinWindowFromID(hDlg, IDC_RADIO_YES);

  switch(msg)
  {
    case WM_INITDLG:
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      WinCheckButton(hDlg, IDC_RADIO_YES, 1);
      WinSetFocus(HWND_DESKTOP, hRadioYes);

      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);

      WinSetDlgItemText(hDlg, 202, sgInstallGui.szSetupMessage);
      WinSetDlgItemText(hDlg, IDC_RADIO_YES, sgInstallGui.szYesRestart);
      WinSetDlgItemText(hDlg, IDC_RADIO_NO, sgInstallGui.szNoRestart);
      WinSetDlgItemText(hDlg, DID_OK, sgInstallGui.szOk);
      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case DID_OK:
          if(WinQueryButtonCheckstate(hDlg, IDC_RADIO_YES) == 1)
          {
            WinDestroyWindow(hDlg);
            WinPostQueueMsg(0, WM_QUIT, 0, 0);
            WinDestroyWindow(hWndMain);

            
            WinShutdownSystem(0, 0);
          }
          else
          {
            WinDestroyWindow(hDlg);
            WinPostQueueMsg(0, WM_QUIT, 0, 0);
          }
          break;

        case IDCANCEL:
          WinDestroyWindow(hDlg);
          WinPostQueueMsg(0, WM_QUIT, 0, 0);
          break;

        default:
          break;
      }
      break;
  }
  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}

MRESULT EXPENTRY DlgProcMessage(HWND hDlg, ULONG msg, MPARAM mp1, MPARAM mp2)
{
  SWP       swpDlg;
  HWND      hSTMessage = WinWindowFromID(hDlg, IDC_MESSAGE); 
  HPS       hpsSTMessage;
  RECTL     rectlString;
  char      szBuf[MAX_BUF];
  char      szBuf2[MAX_BUF];

  memset(szBuf, 0, sizeof(szBuf));
  memset(szBuf2, 0, sizeof(szBuf2));

  switch(msg)
  {
    case WM_INITDLG:
      AdjustDialogSize(hDlg);
      WinSetPresParam(hDlg, PP_FONTNAMESIZE,
                      strlen(sgInstallGui.szDefinedFont)+1, sgInstallGui.szDefinedFont);
      if(GetPrivateProfileString("Messages", "STR_MESSAGEBOX_TITLE", "", szBuf2, sizeof(szBuf2), szFileIniInstall))
      {
        if((sgProduct.szProductName != NULL) && (*sgProduct.szProductName != '\0'))
          sprintf(szBuf, szBuf2, sgProduct.szProductName);
        else
          sprintf(szBuf, szBuf2, "");
      }
      else if((sgProduct.szProductName != NULL) && (*sgProduct.szProductName != '\0'))
        strcpy(szBuf, sgProduct.szProductName);

      WinSetWindowText(hDlg, szBuf);
      WinQueryWindowPos(hDlg, &swpDlg);
      WinSetWindowPos(hDlg,
                      HWND_TOP,
                      (gSystemInfo.lScreenX/2)-(swpDlg.cx/2),
                      (gSystemInfo.lScreenY/2)-(swpDlg.cy/2),
                      0,
                      0,
                      SWP_MOVE);

      break;

    case WM_CLOSE:
      AskCancelDlg(hDlg);
      return (MRESULT)TRUE;
      break;

    case WM_COMMAND:
      switch ( SHORT1FROMMP( mp1 ) )
      {
        case IDC_MESSAGE:
          hpsSTMessage = WinGetPS(hSTMessage);

          rectlString.xLeft = 0;
          rectlString.xRight = 10000;
          rectlString.yTop = 10000;
          rectlString.yBottom = 0;
          WinDrawText(hpsSTMessage, strlen((char*)mp2), (char*)mp2,
                            &rectlString, 0, 0,
                            DT_BOTTOM | DT_QUERYEXTENT | DT_TEXTATTRS | DT_WORDBREAK);
          WinReleasePS(hpsSTMessage);

          WinSetWindowPos(hDlg, HWND_TOP,
                      (gSystemInfo.lScreenX/2)-((rectlString.xRight + 55)/2),
                      (gSystemInfo.lScreenY/2)-((rectlString.yTop + 50)/2),
                      rectlString.xRight + 55,
                      rectlString.yTop + 50,
                      SWP_SIZE | SWP_MOVE);

          WinQueryWindowPos(hDlg, &swpDlg);

          WinSetWindowPos(hSTMessage,
                         HWND_TOP,
                         gSystemInfo.lDlgFrameX,
                         gSystemInfo.lDlgFrameY,
                         swpDlg.cx-2*gSystemInfo.lDlgFrameX,
                         swpDlg.cy-2*gSystemInfo.lDlgFrameY-gSystemInfo.lTitleBarY,
                         SWP_SIZE | SWP_MOVE);

          WinSetDlgItemText(hDlg, IDC_MESSAGE, (PSZ)mp2);
          return (MRESULT)TRUE;
          break;
      }
      break;
  }
  return(WinDefDlgProc(hDlg, msg, mp1, mp2));
}

void ProcessWindowsMessages()
{
  QMSG qmsg;

  while(WinPeekMsg((HAB)0, &qmsg, 0, 0, 0, PM_REMOVE))
  {
    WinDispatchMsg((HAB)0, &qmsg );
  }
}

void ShowMessage(PSZ szMessage, BOOL bShow)
{
  char szBuf[MAX_BUF];
 
  if(sgProduct.ulMode != SILENT)
  {
    if((bShow) && (hDlgMessage == NULL))
    {
      memset(szBuf, 0, sizeof(szBuf));
      GetPrivateProfileString("Messages", "MB_MESSAGE_STR", "", szBuf, sizeof(szBuf), szFileIniInstall);
      hDlgMessage = InstantiateDialog(hWndMain, DLG_MESSAGE, szBuf, DlgProcMessage);
      WinSendMsg(hDlgMessage, WM_COMMAND, IDC_MESSAGE, (LPARAM)szMessage);
    }
    else if(!bShow && hDlgMessage)
    {
      WinDestroyWindow(hDlgMessage);
      hDlgMessage = NULL;
    }
  }
}

HWND InstantiateDialog(HWND hParent, ULONG ulDlgID, PSZ szTitle, PFNWP pfnwpDlgProc)
{
  char szBuf[MAX_BUF];
  HWND hDlg = NULL;
  ATOM atom;

  hDlg = WinLoadDlg(HWND_DESKTOP, hParent, pfnwpDlgProc, hSetupRscInst, ulDlgID, NULL);

  if (hDlg == NULL)
  {
    char szEDialogCreate[MAX_BUF];

    if(GetPrivateProfileString("Messages", "ERROR_DIALOG_CREATE", "", szEDialogCreate, sizeof(szEDialogCreate), szFileIniInstall))
    {
      sprintf(szBuf, szEDialogCreate, szTitle);
      PrintError(szBuf, ERROR_CODE_SHOW);
    }
    WinPostQueueMsg(NULL, WM_QUIT, 1, 0);
  }

  atom = WinFindAtom(WinQuerySystemAtomTable(), CLASS_NAME_SETUP_DLG);
  WinSetWindowULong(hDlg, QWL_USER, atom);

  return(hDlg);
}






void SetTurboArgs(void)
{
  char szData[MAX_BUF];
  char szBuf[MAX_BUF];
  char szApp[MAX_BUF];

  if(diQuickLaunch.bTurboModeEnabled)
  {
    
    LogISTurboMode(diQuickLaunch.bTurboMode);
    LogMSTurboMode(diQuickLaunch.bTurboMode);

    if(diQuickLaunch.bTurboMode)
      strcpy( szData, "turbo=yes" );
    else
      strcpy( szData, "turbo=no" );

    sprintf(szApp, "%s %s",
             sgProduct.szProductNameInternal,
             sgProduct.szUserAgent);
    PrfQueryProfileString(HINI_USERPROFILE, szApp, "browserargs", "", szBuf, sizeof(szBuf));

    if ( szBuf[0] != '\0' )
       strcat(szBuf, " ");
    strcat(szBuf, szData);
  }
}

void DlgSequence(int iDirection)
{
  HRESULT hrValue;
  BOOL    bDone = FALSE;

  do
  {
    gbProcessingXpnstallFiles = FALSE;

    if(iDirection == NEXT_DLG)
    {
      switch(ulWizardState)
      {
        case DLG_NONE:
          ulWizardState = DLG_WELCOME;
          break;
        case DLG_WELCOME:
          ulWizardState = DLG_LICENSE;
          break;
        case DLG_LICENSE:
          ulWizardState = DLG_SETUP_TYPE;
          break;
        case DLG_SETUP_TYPE:
          ulWizardState = DLG_SELECT_COMPONENTS;
          break;
        case DLG_SELECT_COMPONENTS:
          ulWizardState = DLG_SELECT_ADDITIONAL_COMPONENTS;
          break;
        case DLG_SELECT_ADDITIONAL_COMPONENTS:
          ulWizardState = DLG_OS2_INTEGRATION;
          break;
        case DLG_OS2_INTEGRATION:
          ulWizardState = DLG_ADDITIONAL_OPTIONS;
          break;
        case DLG_ADDITIONAL_OPTIONS:
          ulWizardState = DLG_START_INSTALL;
          break;
        case DLG_START_INSTALL:
          ulWizardState = DLG_COMMIT_INSTALL;
          break;

        case DLG_ADVANCED_SETTINGS:
          ulWizardState = DLG_ADDITIONAL_OPTIONS;
          break;

        default:
          ulWizardState = DLG_WELCOME;
          break;      }
    }
    else if(iDirection == PREV_DLG)
    {
      switch(ulWizardState)
      {
        case DLG_LICENSE:
          ulWizardState = DLG_WELCOME;
          break;
        case DLG_SETUP_TYPE:
          ulWizardState = DLG_LICENSE;
          break;
        case DLG_SELECT_COMPONENTS:
          ulWizardState = DLG_SETUP_TYPE;
          break;
        case DLG_SELECT_ADDITIONAL_COMPONENTS:
          ulWizardState = DLG_SELECT_COMPONENTS;
          break;
        case DLG_OS2_INTEGRATION:
          ulWizardState = DLG_SELECT_ADDITIONAL_COMPONENTS;
          break;
        case DLG_ADDITIONAL_OPTIONS:
          ulWizardState = DLG_OS2_INTEGRATION;
          break;
        case DLG_START_INSTALL:
          ulWizardState = DLG_ADDITIONAL_OPTIONS;
          break;

        case DLG_ADVANCED_SETTINGS:
          ulWizardState = DLG_ADDITIONAL_OPTIONS;
          break;

        default:
          ulWizardState = DLG_WELCOME;
          break;
      }
    }
    else if(iDirection == OTHER_DLG_1)
    {
      switch(ulWizardState)
      {
        case DLG_ADDITIONAL_OPTIONS:
          ulWizardState = DLG_ADVANCED_SETTINGS;
          break;

        
        
        
        
        case DLG_ADVANCED_SETTINGS:
          ulWizardState = DLG_ADDITIONAL_OPTIONS;
          break;
      }
    }

    switch(ulWizardState)
    {
      case DLG_WELCOME:
        if(diWelcome.bShowDialog)
        {
          hDlgCurrent = InstantiateDialog(hWndMain, ulWizardState, diWelcome.szTitle, DlgProcWelcome);
          bDone = TRUE;
        }
        break;

      case DLG_LICENSE:
        if(diLicense.bShowDialog)
        {
          hDlgCurrent = InstantiateDialog(hWndMain, ulWizardState, diLicense.szTitle, DlgProcLicense);
          bDone = TRUE;
        }
        break;

      case DLG_SETUP_TYPE:
        if(diSetupType.bShowDialog)
        {
          hDlgCurrent = InstantiateDialog(hWndMain, ulWizardState, diSetupType.szTitle, DlgProcSetupType);
          bDone = TRUE;
        }
        break;

      case DLG_SELECT_COMPONENTS:
        if((diSelectComponents.bShowDialog) && (sgProduct.ulCustomType == ulSetupType))
        {
          hDlgCurrent = InstantiateDialog(hWndMain, ulWizardState, diSelectComponents.szTitle, DlgProcSelectComponents);
          bDone = TRUE;
        }
        break;

      case DLG_SELECT_ADDITIONAL_COMPONENTS:
        if((diSelectAdditionalComponents.bShowDialog) && (sgProduct.ulCustomType == ulSetupType))
        {
          hDlgCurrent = InstantiateDialog(hWndMain, ulWizardState, diSelectAdditionalComponents.szTitle, DlgProcSelectAdditionalComponents);
          bDone = TRUE;
        }
        break;

      case DLG_OS2_INTEGRATION:
        if(diOS2Integration.bShowDialog)
        {
          hDlgCurrent = InstantiateDialog(hWndMain, ulWizardState, diOS2Integration.szTitle, DlgProcOS2Integration);
          bDone = TRUE;
        }
        break;

      case DLG_ADVANCED_SETTINGS:
        if(diAdvancedSettings.bShowDialog)
        {
          hDlgCurrent = InstantiateDialog(hWndMain, ulWizardState, diAdvancedSettings.szTitle, DlgProcAdvancedSettings);
          bDone = TRUE;
        }
        break;

      case DLG_ADDITIONAL_OPTIONS:
        do
        {
          hrValue = VerifyDiskSpace();
          if(hrValue == DID_OK)
          {
            
            iDirection = PREV_DLG;
          }
          else if(hrValue == IDCANCEL)
          {
            AskCancelDlg(hWndMain);
            hrValue = MBID_RETRY;
          }
        }while(hrValue == MBID_RETRY);

        if(hrValue != DID_OK)
        {
          if(ShowAdditionalOptionsDialog() == TRUE)
          {
            hDlgCurrent = InstantiateDialog(hWndMain, ulWizardState, diAdditionalOptions.szTitle, DlgProcAdditionalOptions);
            bDone = TRUE;
          }
        }
        break;

      case DLG_START_INSTALL:
        if(diStartInstall.bShowDialog)
        {
          hDlgCurrent = InstantiateDialog(hWndMain, ulWizardState, diStartInstall.szTitle, DlgProcStartInstall);
          bDone = TRUE;
        }
        break;

      case DLG_COMMIT_INSTALL:
        gbProcessingXpnstallFiles = TRUE;
        CommitInstall();
        bDone = TRUE;
        break;
    }
  } while(!bDone);
}

void CommitInstall(void)
{
  HRESULT hrErr;
  char    szDestPath[MAX_BUF];
  char    szInstallLogFile[MAX_BUF];

        LogISDestinationPath();
        LogISSetupType();
        LogISComponentsSelected();
        LogISComponentsToDownload();
        LogISDiskSpace(gdsnComponentDSRequirement);

        strcpy(szDestPath, sgProduct.szPath);
        if(*sgProduct.szSubPath != '\0')
        {
          AppendBackSlash(szDestPath, sizeof(szDestPath));
          strcat(szDestPath, sgProduct.szSubPath);
        }
        AppendBackSlash(szDestPath, sizeof(szDestPath));

        





        gbILUseTemp = FALSE;

        



        strcpy(szInstallLogFile, szTempDir);
        AppendBackSlash(szInstallLogFile, sizeof(szInstallLogFile));
        strcat(szInstallLogFile, FILE_INSTALL_LOG);
        FileCopy(szInstallLogFile, szDestPath, FALSE, FALSE);
        DosDelete(szInstallLogFile);

        



        strcpy(szInstallLogFile, szTempDir);
        AppendBackSlash(szInstallLogFile, sizeof(szInstallLogFile));
        strcat(szInstallLogFile, FILE_INSTALL_STATUS_LOG);
        FileCopy(szInstallLogFile, szDestPath, FALSE, FALSE);
        DosDelete(szInstallLogFile);

        
        ProcessFileOpsForAll(T_PRE_DOWNLOAD);

        if(RetrieveArchives() == WIZ_OK)
        {
          

          SetTurboArgs();

          if(gbDownloadTriggered || gbPreviousUnfinishedDownload)
            SetSetupState(SETUP_STATE_UNPACK_XPCOM);

          
          ProcessFileOpsForAll(T_POST_DOWNLOAD);
          
          ProcessFileOpsForAll(T_PRE_XPCOM);

          if(ProcessXpcomFile() != FO_SUCCESS)
          {
            bSDUserCanceled = TRUE;
            CleanupXpcomFile();
            WinPostQueueMsg(0, WM_QUIT, 0, 0);

            return;
          }

          if(gbDownloadTriggered || gbPreviousUnfinishedDownload)
            SetSetupState(SETUP_STATE_INSTALL_XPI); 

          
          ProcessFileOpsForAll(T_POST_XPCOM);
          
          ProcessFileOpsForAll(T_PRE_SMARTUPDATE);

          
          if(diAdditionalOptions.bSaveInstaller)
            SaveInstallerFiles();

          if(CheckInstances())
          {
            bSDUserCanceled = TRUE;
            CleanupXpcomFile();
            WinPostQueueMsg(0, WM_QUIT, 0, 0);

            return;
          }

          strcat(szDestPath, "uninstall\\");
          CreateDirectoriesAll(szDestPath, TRUE);

          
          if(diAdditionalOptions.bSaveInstaller)
            SaveInstallerFiles();

          hrErr = SmartUpdateJars();
          if((hrErr == WIZ_OK) || (hrErr == 999))
          {
            UpdateJSProxyInfo();

            
            ProcessFileOpsForAll(T_POST_SMARTUPDATE);
            
            ProcessFileOpsForAll(T_PRE_LAUNCHAPP);

            LaunchApps();

            
            ProcessFileOpsForAll(T_POST_LAUNCHAPP);
            
            ProcessFileOpsForAll(T_DEPEND_REBOOT);

            ProcessOS2Integration();

            UnsetSetupState(); 
            if(!gbIgnoreProgramFolderX)
              ProcessProgramFolderShowCmd();

            CleanupArgsRegistry();
            CleanupPreviousVersionINIKeys();
            if(NeedReboot())
            {
              CleanupXpcomFile();
              hDlgCurrent = InstantiateDialog(hWndMain, DLG_RESTART, diReboot.szTitle, DlgProcReboot);
            }
            else
            {
              CleanupXpcomFile();
              WinPostQueueMsg(0, WM_QUIT, 0, 0);
            }
          }
          else
          {
            CleanupXpcomFile();
            WinPostQueueMsg(0, WM_QUIT, 0, 0);
          }
        }
        else
        {
          bSDUserCanceled = TRUE;
          CleanupXpcomFile();
          CleanupArgsRegistry();
          WinPostQueueMsg(0, WM_QUIT, 0, 0);
        }
        gbProcessingXpnstallFiles = FALSE;
}

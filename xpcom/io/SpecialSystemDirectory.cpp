









































#include "SpecialSystemDirectory.h"
#include "nsString.h"
#include "nsDependentString.h"

#if defined(XP_WIN)

#include <windows.h>
#include <shlobj.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <direct.h>

#elif defined(XP_OS2)

#define MAX_PATH _MAX_PATH
#define INCL_WINWORKPLACE
#define INCL_DOSMISC
#define INCL_DOSMODULEMGR
#define INCL_DOSPROCESS
#define INCL_WINSHELLDATA
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include "prenv.h"

#elif defined(XP_UNIX)

#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/param.h>
#include "prenv.h"

#endif

#if defined(VMS)
#include <unixlib.h>
#endif

#ifndef MAXPATHLEN
#ifdef PATH_MAX
#define MAXPATHLEN PATH_MAX
#elif defined(MAX_PATH)
#define MAXPATHLEN MAX_PATH
#elif defined(_MAX_PATH)
#define MAXPATHLEN _MAX_PATH
#elif defined(CCHMAXPATH)
#define MAXPATHLEN CCHMAXPATH
#else
#define MAXPATHLEN 1024
#endif
#endif

#ifdef XP_WIN
typedef HRESULT (WINAPI* nsGetKnownFolderPath)(GUID& rfid,
                                               DWORD dwFlags,
                                               HANDLE hToken,
                                               PWSTR *ppszPath);

static nsGetKnownFolderPath gGetKnownFolderPath = NULL;
#endif

void StartupSpecialSystemDirectory()
{
#if defined (XP_WIN)
    
    
    HMODULE hShell32DLLInst = GetModuleHandleW(L"shell32.dll");
    if(hShell32DLLInst)
    {
        gGetKnownFolderPath = (nsGetKnownFolderPath)
            GetProcAddress(hShell32DLLInst, "SHGetKnownFolderPath");
    }
#endif
}

#if defined (XP_WIN)

static nsresult GetKnownFolder(GUID* guid, nsILocalFile** aFile)
{
    if (!guid || !gGetKnownFolderPath)
        return NS_ERROR_FAILURE;

    PWSTR path = NULL;
    gGetKnownFolderPath(*guid, 0, NULL, &path);

    if (!path)
        return NS_ERROR_FAILURE;

    nsresult rv = NS_NewLocalFile(nsDependentString(path),
                                  true,
                                  aFile);

    CoTaskMemFree(path);
    return rv;
}


static nsresult GetWindowsFolder(int folder, nsILocalFile** aFile)

{
    WCHAR path_orig[MAX_PATH + 3];
    WCHAR *path = path_orig+1;
    HRESULT result = SHGetSpecialFolderPathW(NULL, path, folder, true);

    if (!SUCCEEDED(result))
        return NS_ERROR_FAILURE;

    
    int len = wcslen(path);
    if (len > 1 && path[len - 1] != L'\\')
    {
        path[len]   = L'\\';
        path[++len] = L'\0';
    }

    return NS_NewLocalFile(nsDependentString(path, len), true, aFile);
}






static nsresult GetRegWindowsAppDataFolder(bool aLocal, nsILocalFile** aFile)
{
    HKEY key;
    NS_NAMED_LITERAL_STRING(keyName,
    "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders");
    DWORD res = ::RegOpenKeyExW(HKEY_CURRENT_USER, keyName.get(), 0, KEY_READ,
                                &key);
    if (res != ERROR_SUCCESS)
        return NS_ERROR_FAILURE;

    WCHAR path[MAX_PATH + 2];
    DWORD type, size;
    res = RegQueryValueExW(key, (aLocal ? L"Local AppData" : L"AppData"), NULL,
                           &type, (LPBYTE)&path, &size);
    ::RegCloseKey(key);
    
    
    if (res != ERROR_SUCCESS || type != REG_SZ || size == 0 || size % 2 != 0)
        return NS_ERROR_FAILURE;

    
    int len = wcslen(path);
    if (len > 1 && path[len - 1] != L'\\')
    {
        path[len]   = L'\\';
        path[++len] = L'\0';
    }

    return NS_NewLocalFile(nsDependentString(path, len), true, aFile);
}

#endif 

#if defined(XP_UNIX)
static nsresult
GetUnixHomeDir(nsILocalFile** aFile)
{
#ifdef VMS
    char *pHome;
    pHome = getenv("HOME");
    if (*pHome == '/') {
        return NS_NewNativeLocalFile(nsDependentCString(pHome),
                                     true,
                                     aFile);
    } else {
        return NS_NewNativeLocalFile(nsDependentCString(decc$translate_vms(pHome)),
                                     true,
                                     aFile);
    }
#elif defined(ANDROID)
    
    return NS_ERROR_FAILURE;
#else
    return NS_NewNativeLocalFile(nsDependentCString(PR_GetEnv("HOME")),
                                 true, aFile);
#endif
}



























static char *
xdg_user_dir_lookup (const char *type)
{
  FILE *file;
  char *home_dir, *config_home, *config_file;
  char buffer[512];
  char *user_dir;
  char *p, *d;
  int len;
  int relative;

  home_dir = getenv ("HOME");

  if (home_dir == NULL)
    goto error;

  config_home = getenv ("XDG_CONFIG_HOME");
  if (config_home == NULL || config_home[0] == 0)
    {
      config_file = (char*) malloc (strlen (home_dir) + strlen ("/.config/user-dirs.dirs") + 1);
      if (config_file == NULL)
        goto error;

      strcpy (config_file, home_dir);
      strcat (config_file, "/.config/user-dirs.dirs");
    }
  else
    {
      config_file = (char*) malloc (strlen (config_home) + strlen ("/user-dirs.dirs") + 1);
      if (config_file == NULL)
        goto error;

      strcpy (config_file, config_home);
      strcat (config_file, "/user-dirs.dirs");
    }

  file = fopen (config_file, "r");
  free (config_file);
  if (file == NULL)
    goto error;

  user_dir = NULL;
  while (fgets (buffer, sizeof (buffer), file))
    {
      
      len = strlen (buffer);
      if (len > 0 && buffer[len-1] == '\n')
	buffer[len-1] = 0;

      p = buffer;
      while (*p == ' ' || *p == '\t')
	p++;

      if (strncmp (p, "XDG_", 4) != 0)
	continue;
      p += 4;
      if (strncmp (p, type, strlen (type)) != 0)
	continue;
      p += strlen (type);
      if (strncmp (p, "_DIR", 4) != 0)
	continue;
      p += 4;

      while (*p == ' ' || *p == '\t')
	p++;

      if (*p != '=')
	continue;
      p++;

      while (*p == ' ' || *p == '\t')
	p++;

      if (*p != '"')
	continue;
      p++;

      relative = 0;
      if (strncmp (p, "$HOME/", 6) == 0)
	{
	  p += 6;
	  relative = 1;
	}
      else if (*p != '/')
	continue;

      if (relative)
	{
	  user_dir = (char*) malloc (strlen (home_dir) + 1 + strlen (p) + 1);
          if (user_dir == NULL)
            goto error2;

	  strcpy (user_dir, home_dir);
	  strcat (user_dir, "/");
	}
      else
	{
	  user_dir = (char*) malloc (strlen (p) + 1);
          if (user_dir == NULL)
            goto error2;

	  *user_dir = 0;
	}

      d = user_dir + strlen (user_dir);
      while (*p && *p != '"')
	{
	  if ((*p == '\\') && (*(p+1) != 0))
	    p++;
	  *d++ = *p++;
	}
      *d = 0;
    }
error2:
  fclose (file);

  if (user_dir)
    return user_dir;

 error:
  return NULL;
}

static const char xdg_user_dirs[] =
    "DESKTOP\0"
    "DOCUMENTS\0"
    "DOWNLOAD\0"
    "MUSIC\0"
    "PICTURES\0"
    "PUBLICSHARE\0"
    "TEMPLATES\0"
    "VIDEOS";

static const PRUint8 xdg_user_dir_offsets[] = {
    0,
    8,
    18,
    27,
    33,
    42,
    54,
    64
};

static nsresult
GetUnixXDGUserDirectory(SystemDirectories aSystemDirectory,
                        nsILocalFile** aFile)
{
    char *dir = xdg_user_dir_lookup
                    (xdg_user_dirs + xdg_user_dir_offsets[aSystemDirectory -
                                                         Unix_XDG_Desktop]);

    nsresult rv;
    nsCOMPtr<nsILocalFile> file;
    if (dir) {
        rv = NS_NewNativeLocalFile(nsDependentCString(dir), true,
                                   getter_AddRefs(file));
        free(dir);
    } else if (Unix_XDG_Desktop == aSystemDirectory) {
        
        
        rv = GetUnixHomeDir(getter_AddRefs(file));
        if (NS_FAILED(rv))
            return rv;

        rv = file->AppendNative(NS_LITERAL_CSTRING("Desktop"));
    }
#if defined(MOZ_PLATFORM_MAEMO)
    
    else if (Unix_XDG_Documents == aSystemDirectory) {

        char *myDocs = PR_GetEnv("MYDOCSDIR");
        if (!myDocs || !*myDocs)
            return NS_ERROR_FAILURE;

        rv = NS_NewNativeLocalFile(nsDependentCString(myDocs), true,
                                   getter_AddRefs(file));
        if (NS_FAILED(rv))
            return rv;

        rv = file->AppendNative(NS_LITERAL_CSTRING(".documents"));
    }
#endif
    else {
      
      rv = NS_ERROR_FAILURE;
    }

    if (NS_FAILED(rv))
        return rv;

    bool exists;
    rv = file->Exists(&exists);
    if (NS_FAILED(rv))
        return rv;
    if (!exists) {
        rv = file->Create(nsIFile::DIRECTORY_TYPE, 0755);
        if (NS_FAILED(rv))
            return rv;
    }

    *aFile = nsnull;
    file.swap(*aFile);

    return NS_OK;
}
#endif

nsresult
GetSpecialSystemDirectory(SystemDirectories aSystemSystemDirectory,
                          nsILocalFile** aFile)
{
#if defined(XP_WIN)
    WCHAR path[MAX_PATH];
#else
    char path[MAXPATHLEN];
#endif

    switch (aSystemSystemDirectory)
    {
        case OS_CurrentWorkingDirectory:
#if defined(XP_WIN)
            if (!_wgetcwd(path, MAX_PATH))
                return NS_ERROR_FAILURE;
            return NS_NewLocalFile(nsDependentString(path),
                                   true,
                                   aFile);
#elif defined(XP_OS2)
            if (DosQueryPathInfo( ".", FIL_QUERYFULLNAME, path, MAXPATHLEN))
                return NS_ERROR_FAILURE;
#else
            if(!getcwd(path, MAXPATHLEN))
                return NS_ERROR_FAILURE;
#endif

#if !defined(XP_WIN)
            return NS_NewNativeLocalFile(nsDependentCString(path),
                                         true,
                                         aFile);
#endif

        case OS_DriveDirectory:
#if defined (XP_WIN)
        {
            PRInt32 len = ::GetWindowsDirectoryW(path, MAX_PATH);
            if (len == 0)
                break;
            if (path[1] == PRUnichar(':') && path[2] == PRUnichar('\\'))
                path[3] = 0;

            return NS_NewLocalFile(nsDependentString(path),
                                   true,
                                   aFile);
        }
#elif defined(XP_OS2)
        {
            ULONG ulBootDrive = 0;
            char  buffer[] = " :\\OS2\\";
            DosQuerySysInfo( QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
                             &ulBootDrive, sizeof ulBootDrive);
            buffer[0] = 'A' - 1 + ulBootDrive; 

            return NS_NewNativeLocalFile(nsDependentCString(buffer),
                                         true,
                                         aFile);
        }
#else
        return NS_NewNativeLocalFile(nsDependentCString("/"),
                                     true,
                                     aFile);

#endif

        case OS_TemporaryDirectory:
#if defined (XP_WIN)
            {
            DWORD len = ::GetTempPathW(MAX_PATH, path);
            if (len == 0)
                break;
            return NS_NewLocalFile(nsDependentString(path, len),
                                   true,
                                   aFile);
        }
#elif defined(XP_OS2)
        {
            char *tPath = PR_GetEnv("TMP");
            if (!tPath || !*tPath) {
                tPath = PR_GetEnv("TEMP");
                if (!tPath || !*tPath) {
                    
                    
                    return NS_ERROR_UNEXPECTED;
                }
            }
            nsCString tString = nsDependentCString(tPath);
            if (tString.Find("/", false, 0, -1)) {
                tString.ReplaceChar('/','\\');
            }
            return NS_NewNativeLocalFile(tString, true, aFile);
        }
#elif defined(MOZ_WIDGET_COCOA)
        {
            return GetOSXFolderType(kUserDomain, kTemporaryFolderType, aFile);
        }

#elif defined(XP_UNIX)
        {
            static const char *tPath = nsnull;
            if (!tPath) {
                tPath = PR_GetEnv("TMPDIR");
                if (!tPath || !*tPath) {
                    tPath = PR_GetEnv("TMP");
                    if (!tPath || !*tPath) {
                        tPath = PR_GetEnv("TEMP");
                        if (!tPath || !*tPath) {
                            tPath = "/tmp/";
                        }
                    }
                }
            }
            return NS_NewNativeLocalFile(nsDependentCString(tPath),
                                         true,
                                         aFile);
        }
#else
        break;
#endif
#if defined (XP_WIN)
        case Win_SystemDirectory:
        {
            PRInt32 len = ::GetSystemDirectoryW(path, MAX_PATH);

            
            if (!len || len > MAX_PATH - 2)
                break;
            path[len]   = L'\\';
            path[++len] = L'\0';

            return NS_NewLocalFile(nsDependentString(path, len),
                                   true,
                                   aFile);
        }

        case Win_WindowsDirectory:
        {
            PRInt32 len = ::GetWindowsDirectoryW(path, MAX_PATH);

            
            if (!len || len > MAX_PATH - 2)
                break;

            path[len]   = L'\\';
            path[++len] = L'\0';

            return NS_NewLocalFile(nsDependentString(path, len),
                                   true,
                                   aFile);
        }

        case Win_ProgramFiles:
        {
            return GetWindowsFolder(CSIDL_PROGRAM_FILES, aFile);
        }

        case Win_HomeDirectory:
        {
            nsresult rv = GetWindowsFolder(CSIDL_PROFILE, aFile);
            if (NS_SUCCEEDED(rv))
                return rv;

            PRInt32 len;
            if ((len = ::GetEnvironmentVariableW(L"HOME", path, MAX_PATH)) > 0)
            {
                
                if (len > MAX_PATH - 2)
                    break;

                path[len]   = L'\\';
                path[++len] = L'\0';

                rv = NS_NewLocalFile(nsDependentString(path, len),
                                     true,
                                     aFile);
                if (NS_SUCCEEDED(rv))
                    return rv;
            }

            len = ::GetEnvironmentVariableW(L"HOMEDRIVE", path, MAX_PATH);
            if (0 < len && len < MAX_PATH)
            {
                WCHAR temp[MAX_PATH];
                DWORD len2 = ::GetEnvironmentVariableW(L"HOMEPATH", temp, MAX_PATH);
                if (0 < len2 && len + len2 < MAX_PATH)
                    wcsncat(path, temp, len2);

                len = wcslen(path);

                
                if (len > MAX_PATH - 2)
                    break;

                path[len]   = L'\\';
                path[++len] = L'\0';

                return NS_NewLocalFile(nsDependentString(path, len),
                                       true,
                                       aFile);
            }
        }
        case Win_Desktop:
        {
            return GetWindowsFolder(CSIDL_DESKTOP, aFile);
        }
        case Win_Programs:
        {
            return GetWindowsFolder(CSIDL_PROGRAMS, aFile);
        }

        case Win_Downloads:
        {
            
            GUID folderid_downloads = {0x374de290, 0x123f, 0x4565, {0x91, 0x64,
                                       0x39, 0xc4, 0x92, 0x5e, 0x46, 0x7b}};
            nsresult rv = GetKnownFolder(&folderid_downloads, aFile);
            
            
            if(NS_ERROR_FAILURE == rv)
            {
              rv = GetWindowsFolder(CSIDL_DESKTOP, aFile);
            }
            return rv;
        }

        case Win_Controls:
        {
            return GetWindowsFolder(CSIDL_CONTROLS, aFile);
        }
        case Win_Printers:
        {
            return GetWindowsFolder(CSIDL_PRINTERS, aFile);
        }
        case Win_Personal:
        {
            return GetWindowsFolder(CSIDL_PERSONAL, aFile);
        }
        case Win_Favorites:
        {
            return GetWindowsFolder(CSIDL_FAVORITES, aFile);
        }
        case Win_Startup:
        {
            return GetWindowsFolder(CSIDL_STARTUP, aFile);
        }
        case Win_Recent:
        {
            return GetWindowsFolder(CSIDL_RECENT, aFile);
        }
        case Win_Sendto:
        {
            return GetWindowsFolder(CSIDL_SENDTO, aFile);
        }
        case Win_Bitbucket:
        {
            return GetWindowsFolder(CSIDL_BITBUCKET, aFile);
        }
        case Win_Startmenu:
        {
            return GetWindowsFolder(CSIDL_STARTMENU, aFile);
        }
        case Win_Desktopdirectory:
        {
            return GetWindowsFolder(CSIDL_DESKTOPDIRECTORY, aFile);
        }
        case Win_Drives:
        {
            return GetWindowsFolder(CSIDL_DRIVES, aFile);
        }
        case Win_Network:
        {
            return GetWindowsFolder(CSIDL_NETWORK, aFile);
        }
        case Win_Nethood:
        {
            return GetWindowsFolder(CSIDL_NETHOOD, aFile);
        }
        case Win_Fonts:
        {
            return GetWindowsFolder(CSIDL_FONTS, aFile);
        }
        case Win_Templates:
        {
            return GetWindowsFolder(CSIDL_TEMPLATES, aFile);
        }
        case Win_Common_Startmenu:
        {
            return GetWindowsFolder(CSIDL_COMMON_STARTMENU, aFile);
        }
        case Win_Common_Programs:
        {
            return GetWindowsFolder(CSIDL_COMMON_PROGRAMS, aFile);
        }
        case Win_Common_Startup:
        {
            return GetWindowsFolder(CSIDL_COMMON_STARTUP, aFile);
        }
        case Win_Common_Desktopdirectory:
        {
            return GetWindowsFolder(CSIDL_COMMON_DESKTOPDIRECTORY, aFile);
        }
        case Win_Common_AppData:
        {
            return GetWindowsFolder(CSIDL_COMMON_APPDATA, aFile);
        }
        case Win_Printhood:
        {
            return GetWindowsFolder(CSIDL_PRINTHOOD, aFile);
        }
        case Win_Cookies:
        {
            return GetWindowsFolder(CSIDL_COOKIES, aFile);
        }
        case Win_Appdata:
        {
            nsresult rv = GetWindowsFolder(CSIDL_APPDATA, aFile);
            if (NS_FAILED(rv))
                rv = GetRegWindowsAppDataFolder(false, aFile);
            return rv;
        }
        case Win_LocalAppdata:
        {
            nsresult rv = GetWindowsFolder(CSIDL_LOCAL_APPDATA, aFile);
            if (NS_FAILED(rv))
                rv = GetRegWindowsAppDataFolder(true, aFile);
            return rv;
        }
#endif  

#if defined(XP_UNIX)
        case Unix_LocalDirectory:
            return NS_NewNativeLocalFile(nsDependentCString("/usr/local/netscape/"),
                                         true,
                                         aFile);
        case Unix_LibDirectory:
            return NS_NewNativeLocalFile(nsDependentCString("/usr/local/lib/netscape/"),
                                         true,
                                         aFile);

        case Unix_HomeDirectory:
            return GetUnixHomeDir(aFile);

        case Unix_XDG_Desktop:
        case Unix_XDG_Documents:
        case Unix_XDG_Download:
        case Unix_XDG_Music:
        case Unix_XDG_Pictures:
        case Unix_XDG_PublicShare:
        case Unix_XDG_Templates:
        case Unix_XDG_Videos:
            return GetUnixXDGUserDirectory(aSystemSystemDirectory, aFile);
#endif

#ifdef XP_OS2
        case OS2_SystemDirectory:
        {
            ULONG ulBootDrive = 0;
            char  buffer[] = " :\\OS2\\System\\";
            DosQuerySysInfo( QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
                             &ulBootDrive, sizeof ulBootDrive);
            buffer[0] = 'A' - 1 + ulBootDrive; 

            return NS_NewNativeLocalFile(nsDependentCString(buffer),
                                         true,
                                         aFile);
        }

     case OS2_OS2Directory:
        {
            ULONG ulBootDrive = 0;
            char  buffer[] = " :\\OS2\\";
            DosQuerySysInfo( QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
                             &ulBootDrive, sizeof ulBootDrive);
            buffer[0] = 'A' - 1 + ulBootDrive; 

            return NS_NewNativeLocalFile(nsDependentCString(buffer),
                                         true,
                                         aFile);
        }

     case OS2_HomeDirectory:
        {
            nsresult rv;
            char *tPath = PR_GetEnv("MOZILLA_HOME");
            char buffer[CCHMAXPATH];
            
            
            if (!tPath || !*tPath) {
                PPIB ppib;
                PTIB ptib;
                DosGetInfoBlocks( &ptib, &ppib);
                DosQueryModuleName( ppib->pib_hmte, CCHMAXPATH, buffer);
                *strrchr( buffer, '\\') = '\0'; 
                tPath = buffer;
            }
            rv = NS_NewNativeLocalFile(nsDependentCString(tPath),
                                       true,
                                       aFile);

            PrfWriteProfileString(HINI_USERPROFILE, "Mozilla", "Home", tPath);
            return rv;
        }

        case OS2_DesktopDirectory:
        {
            char szPath[CCHMAXPATH + 1];
            BOOL fSuccess;
            fSuccess = WinQueryActiveDesktopPathname (szPath, sizeof(szPath));
            if (!fSuccess) {
                
                
                return GetSpecialSystemDirectory(OS2_HomeDirectory, aFile);
            }
            int len = strlen (szPath);
            if (len > CCHMAXPATH -1)
                break;
            szPath[len] = '\\';
            szPath[len + 1] = '\0';

            return NS_NewNativeLocalFile(nsDependentCString(szPath),
                                         true,
                                         aFile);
        }
#endif
        default:
            break;
    }
    return NS_ERROR_NOT_AVAILABLE;
}

#if defined (MOZ_WIDGET_COCOA)
nsresult
GetOSXFolderType(short aDomain, OSType aFolderType, nsILocalFile **localFile)
{
    OSErr err;
    FSRef fsRef;
    nsresult rv = NS_ERROR_FAILURE;

    err = ::FSFindFolder(aDomain, aFolderType, kCreateFolder, &fsRef);
    if (err == noErr)
    {
        NS_NewLocalFile(EmptyString(), true, localFile);
        nsCOMPtr<nsILocalFileMac> localMacFile(do_QueryInterface(*localFile));
        if (localMacFile)
            rv = localMacFile->InitWithFSRef(&fsRef);
    }
    return rv;
}
#endif


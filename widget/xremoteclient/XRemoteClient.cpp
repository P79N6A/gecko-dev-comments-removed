







#include "mozilla/ArrayUtils.h"
#include "XRemoteClient.h"
#include "prmem.h"
#include "prprf.h"
#include "plstr.h"
#include "prsystem.h"
#include "prlog.h"
#include "prenv.h"
#include "prdtoa.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>
#include <X11/Xatom.h>

#define MOZILLA_VERSION_PROP   "_MOZILLA_VERSION"
#define MOZILLA_LOCK_PROP      "_MOZILLA_LOCK"
#define MOZILLA_COMMANDLINE_PROP "_MOZILLA_COMMANDLINE"
#define MOZILLA_RESPONSE_PROP  "_MOZILLA_RESPONSE"
#define MOZILLA_USER_PROP      "_MOZILLA_USER"
#define MOZILLA_PROFILE_PROP   "_MOZILLA_PROFILE"
#define MOZILLA_PROGRAM_PROP   "_MOZILLA_PROGRAM"

#ifdef IS_BIG_ENDIAN
#define TO_LITTLE_ENDIAN32(x) \
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) | \
    (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))
#else
#define TO_LITTLE_ENDIAN32(x) (x)
#endif
    
#ifndef MAX_PATH
#ifdef PATH_MAX
#define MAX_PATH PATH_MAX
#else
#define MAX_PATH 1024
#endif
#endif

static PRLogModuleInfo *sRemoteLm = nullptr;

static int (*sOldHandler)(Display *, XErrorEvent *);
static bool sGotBadWindow;

XRemoteClient::XRemoteClient()
{
  mDisplay = 0;
  mInitialized = false;
  mMozVersionAtom = 0;
  mMozLockAtom = 0;
  mMozResponseAtom = 0;
  mMozWMStateAtom = 0;
  mMozUserAtom = 0;
  mLockData = 0;
  if (!sRemoteLm)
    sRemoteLm = PR_NewLogModule("XRemoteClient");
  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("XRemoteClient::XRemoteClient"));
}

XRemoteClient::~XRemoteClient()
{
  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("XRemoteClient::~XRemoteClient"));
  if (mInitialized)
    Shutdown();
}


static const char *XAtomNames[] = {
  MOZILLA_VERSION_PROP,
  MOZILLA_LOCK_PROP,
  MOZILLA_RESPONSE_PROP,
  "WM_STATE",
  MOZILLA_USER_PROP,
  MOZILLA_PROFILE_PROP,
  MOZILLA_PROGRAM_PROP,
  MOZILLA_COMMANDLINE_PROP
};
static Atom XAtoms[MOZ_ARRAY_LENGTH(XAtomNames)];

nsresult
XRemoteClient::Init()
{
  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("XRemoteClient::Init"));

  if (mInitialized)
    return NS_OK;

  
  mDisplay = XOpenDisplay(0);
  if (!mDisplay)
    return NS_ERROR_FAILURE;

  
  XInternAtoms(mDisplay, const_cast<char**>(XAtomNames),
               MOZ_ARRAY_LENGTH(XAtomNames), False, XAtoms);

  int i = 0;
  mMozVersionAtom  = XAtoms[i++];
  mMozLockAtom     = XAtoms[i++];
  mMozResponseAtom = XAtoms[i++];
  mMozWMStateAtom  = XAtoms[i++];
  mMozUserAtom     = XAtoms[i++];
  mMozProfileAtom  = XAtoms[i++];
  mMozProgramAtom  = XAtoms[i++];
  mMozCommandLineAtom = XAtoms[i++];

  mInitialized = true;

  return NS_OK;
}

void
XRemoteClient::Shutdown (void)
{
  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("XRemoteClient::Shutdown"));

  if (!mInitialized)
    return;

  
  XCloseDisplay(mDisplay);
  mDisplay = 0;
  mInitialized = false;
  if (mLockData) {
    free(mLockData);
    mLockData = 0;
  }
}

static int
HandleBadWindow(Display *display, XErrorEvent *event)
{
  if (event->error_code == BadWindow) {
    sGotBadWindow = true;
    return 0; 
  }
  else {
    return (*sOldHandler)(display, event);
  }
}

nsresult
XRemoteClient::SendCommandLine (const char *aProgram, const char *aUsername,
                                const char *aProfile,
                                int32_t argc, char **argv,
                                const char* aDesktopStartupID,
                                char **aResponse, bool *aWindowFound)
{
  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("XRemoteClient::SendCommandLine"));

  *aWindowFound = false;

  
  
  sOldHandler = XSetErrorHandler(HandleBadWindow);

  Window w = FindBestWindow(aProgram, aUsername, aProfile);

  nsresult rv = NS_OK;

  if (w) {
    
    *aWindowFound = true;

    
    
    
    sGotBadWindow = false;

    
    XSelectInput(mDisplay, w,
                 (PropertyChangeMask|StructureNotifyMask));

    bool destroyed = false;

    
    rv = GetLock(w, &destroyed);

    if (NS_SUCCEEDED(rv)) {
      
      rv = DoSendCommandLine(w, argc, argv, aDesktopStartupID, aResponse,
                             &destroyed);

      
      
      if (!destroyed)
          FreeLock(w); 

    }
  }

  XSetErrorHandler(sOldHandler);

  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("SendCommandInternal returning 0x%x\n", rv));

  return rv;
}

Window
XRemoteClient::CheckWindow(Window aWindow)
{
  Atom type = None;
  int  format;
  unsigned long nitems, bytesafter;
  unsigned char *data;
  Window innerWindow;

  XGetWindowProperty(mDisplay, aWindow, mMozWMStateAtom,
		     0, 0, False, AnyPropertyType,
		     &type, &format, &nitems, &bytesafter, &data);

  if (type) {
    XFree(data);
    return aWindow;
  }

  
  innerWindow = CheckChildren(aWindow);

  if (innerWindow)
    return innerWindow;

  return aWindow;
}

Window
XRemoteClient::CheckChildren(Window aWindow)
{
  Window root, parent;
  Window *children;
  unsigned int nchildren;
  unsigned int i;
  Atom type = None;
  int format;
  unsigned long nitems, after;
  unsigned char *data;
  Window retval = None;
  
  if (!XQueryTree(mDisplay, aWindow, &root, &parent, &children,
		  &nchildren))
    return None;
  
  
  
  for (i=0; !retval && (i < nchildren); i++) {
    XGetWindowProperty(mDisplay, children[i], mMozWMStateAtom,
		       0, 0, False, AnyPropertyType, &type, &format,
		       &nitems, &after, &data);
    if (type) {
      XFree(data);
      retval = children[i];
    }
  }

  
  for (i=0; !retval && (i < nchildren); i++) {
    retval = CheckChildren(children[i]);
  }

  if (children)
    XFree((char *)children);

  return retval;
}

nsresult
XRemoteClient::GetLock(Window aWindow, bool *aDestroyed)
{
  bool locked = false;
  bool waited = false;
  *aDestroyed = false;

  nsresult rv = NS_OK;

  if (!mLockData) {
    
    char pidstr[32];
    char sysinfobuf[SYS_INFO_BUFFER_LENGTH];
    PR_snprintf(pidstr, sizeof(pidstr), "pid%d@", getpid());
    PRStatus status;
    status = PR_GetSystemInfo(PR_SI_HOSTNAME, sysinfobuf,
			      SYS_INFO_BUFFER_LENGTH);
    if (status != PR_SUCCESS) {
      return NS_ERROR_FAILURE;
    }
    
    
    
    mLockData = (char *)malloc(strlen(pidstr) + strlen(sysinfobuf) + 1);
    if (!mLockData)
      return NS_ERROR_OUT_OF_MEMORY;

    strcpy(mLockData, pidstr);
    if (!strcat(mLockData, sysinfobuf))
      return NS_ERROR_FAILURE;
  }

  do {
    int result;
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *data = 0;

    XGrabServer(mDisplay);

    result = XGetWindowProperty (mDisplay, aWindow, mMozLockAtom,
				 0, (65536 / sizeof (long)),
				 False, 
				 XA_STRING,
				 &actual_type, &actual_format,
				 &nitems, &bytes_after,
				 &data);

    
    
    
    
    if (sGotBadWindow) {
      *aDestroyed = true;
      rv = NS_ERROR_FAILURE;
    }
    else if (result != Success || actual_type == None) {
      
      XChangeProperty (mDisplay, aWindow, mMozLockAtom, XA_STRING, 8,
		       PropModeReplace,
		       (unsigned char *)mLockData,
		       strlen(mLockData));
      locked = True;
    }

    XUngrabServer(mDisplay);
    XFlush(mDisplay); 

    if (!locked && !NS_FAILED(rv)) {
      


      PR_LOG(sRemoteLm, PR_LOG_DEBUG, 
	     ("window 0x%x is locked by %s; waiting...\n",
	      (unsigned int) aWindow, data));
      waited = True;
      while (1) {
	XEvent event;
	int select_retval;
	fd_set select_set;
	struct timeval delay;
	delay.tv_sec = 10;
	delay.tv_usec = 0;

	FD_ZERO(&select_set);
	
	FD_SET(ConnectionNumber(mDisplay), &select_set);
	select_retval = select(ConnectionNumber(mDisplay) + 1,
			       &select_set, nullptr, nullptr, &delay);
	
	if (select_retval == 0) {
	  PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("timed out waiting for window\n"));
          rv = NS_ERROR_FAILURE;
          break;
	}
	PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("xevent...\n"));
	XNextEvent (mDisplay, &event);
	if (event.xany.type == DestroyNotify &&
	    event.xdestroywindow.window == aWindow) {
	  *aDestroyed = true;
          rv = NS_ERROR_FAILURE;
          break;
	}
	else if (event.xany.type == PropertyNotify &&
		 event.xproperty.state == PropertyDelete &&
		 event.xproperty.window == aWindow &&
		 event.xproperty.atom == mMozLockAtom) {
	  

	  PR_LOG(sRemoteLm, PR_LOG_DEBUG,
		 ("(0x%x unlocked, trying again...)\n",
		  (unsigned int) aWindow));
		  break;
	}
      }
    }
    if (data)
      XFree(data);
  } while (!locked && !NS_FAILED(rv));

  if (waited && locked) {
    PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("obtained lock.\n"));
  } else if (*aDestroyed) {
    PR_LOG(sRemoteLm, PR_LOG_DEBUG,
           ("window 0x%x unexpectedly destroyed.\n",
            (unsigned int) aWindow));
  }

  return rv;
}

Window
XRemoteClient::FindBestWindow(const char *aProgram, const char *aUsername,
                              const char *aProfile)
{
  Window root = RootWindowOfScreen(DefaultScreenOfDisplay(mDisplay));
  Window bestWindow = 0;
  Window root2, parent, *kids;
  unsigned int nkids;

  
  
  if (!XQueryTree(mDisplay, root, &root2, &parent, &kids, &nkids)) {
    PR_LOG(sRemoteLm, PR_LOG_DEBUG,
           ("XQueryTree failed in XRemoteClient::FindBestWindow"));
    return 0;
  }

  if (!(kids && nkids)) {
    PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("root window has no children"));
    return 0;
  }

  
  

  for (unsigned int i = 0; i < nkids; i++) {
    Atom type;
    int format;
    unsigned long nitems, bytesafter;
    unsigned char *data_return = 0;
    Window w;
    w = kids[i];
    
    w = CheckWindow(w);

    int status = XGetWindowProperty(mDisplay, w, mMozVersionAtom,
                                    0, (65536 / sizeof (long)),
                                    False, XA_STRING,
                                    &type, &format, &nitems, &bytesafter,
                                    &data_return);

    if (!data_return)
      continue;

    double version = PR_strtod((char*) data_return, nullptr);
    XFree(data_return);

    if (!(version >= 5.1 && version < 6))
      continue;

    data_return = 0;

    if (status != Success || type == None)
      continue;

    
    
    
    
    if (aProgram && strcmp(aProgram, "any")) {
        status = XGetWindowProperty(mDisplay, w, mMozProgramAtom,
                                    0, (65536 / sizeof(long)),
                                    False, XA_STRING,
                                    &type, &format, &nitems, &bytesafter,
                                    &data_return);
        
        
        
        if (data_return) {
            if (strcmp(aProgram, (const char *)data_return)) {
                XFree(data_return);
                continue;
            }

            
            XFree(data_return);
        }
        else {
            
            
            continue;
        }
    }

    
    
    const char *username;
    if (aUsername) {
      username = aUsername;
    }
    else {
      username = PR_GetEnv("LOGNAME");
    }

    if (username) {
        status = XGetWindowProperty(mDisplay, w, mMozUserAtom,
                                    0, (65536 / sizeof(long)),
                                    False, XA_STRING,
                                    &type, &format, &nitems, &bytesafter,
                                    &data_return);

        
        if (data_return) {
            
            if (strcmp(username, (const char *)data_return)) {
                XFree(data_return);
                continue;
            }

            XFree(data_return);
        }
    }

    
    
    
    if (aProfile) {
        status = XGetWindowProperty(mDisplay, w, mMozProfileAtom,
                                    0, (65536 / sizeof(long)),
                                    False, XA_STRING,
                                    &type, &format, &nitems, &bytesafter,
                                    &data_return);

        
        if (data_return) {
            
            if (strcmp(aProfile, (const char *)data_return)) {
                XFree(data_return);
                continue;
            }

            XFree(data_return);
        }
    }

    
    

    
    
    bestWindow = w;
    break;
  }

  if (kids)
    XFree((char *) kids);

  return bestWindow;
}

nsresult
XRemoteClient::FreeLock(Window aWindow)
{
  int result;
  Atom actual_type;
  int actual_format;
  unsigned long nitems, bytes_after;
  unsigned char *data = 0;

  result = XGetWindowProperty(mDisplay, aWindow, mMozLockAtom,
                              0, (65536 / sizeof(long)),
                              True, 
                              XA_STRING,
                              &actual_type, &actual_format,
                              &nitems, &bytes_after,
                              &data);
  if (result != Success) {
      PR_LOG(sRemoteLm, PR_LOG_DEBUG,
             ("unable to read and delete " MOZILLA_LOCK_PROP
              " property\n"));
      return NS_ERROR_FAILURE;
  }
  else if (!data || !*data){
      PR_LOG(sRemoteLm, PR_LOG_DEBUG,
             ("invalid data on " MOZILLA_LOCK_PROP
              " of window 0x%x.\n",
              (unsigned int) aWindow));
      return NS_ERROR_FAILURE;
  }
  else if (strcmp((char *)data, mLockData)) {
      PR_LOG(sRemoteLm, PR_LOG_DEBUG,
             (MOZILLA_LOCK_PROP " was stolen!  Expected \"%s\", saw \"%s\"!\n",
              mLockData, data));
      return NS_ERROR_FAILURE;
  }

  if (data)
      XFree(data);
  return NS_OK;
}


static char*
estrcpy(const char* s, char* d)
{
  while (*s)
    *d++ = *s++;

  *d++ = '\0';
  return d;
}

nsresult
XRemoteClient::DoSendCommandLine(Window aWindow, int32_t argc, char **argv,
                                 const char* aDesktopStartupID,
                                 char **aResponse, bool *aDestroyed)
{
  *aDestroyed = false;

  char cwdbuf[MAX_PATH];
  if (!getcwd(cwdbuf, MAX_PATH))
    return NS_ERROR_UNEXPECTED;

  
  
  
  
  

  static char desktopStartupPrefix[] = " DESKTOP_STARTUP_ID=";

  int32_t argvlen = strlen(cwdbuf);
  for (int i = 0; i < argc; ++i) {
    int32_t len = strlen(argv[i]);
    if (i == 0 && aDesktopStartupID) {
      len += sizeof(desktopStartupPrefix) - 1 + strlen(aDesktopStartupID);
    }
    argvlen += len;
  }

  int32_t* buffer = (int32_t*) malloc(argvlen + argc + 1 +
                                      sizeof(int32_t) * (argc + 1));
  if (!buffer)
    return NS_ERROR_OUT_OF_MEMORY;

  buffer[0] = TO_LITTLE_ENDIAN32(argc);

  char *bufend = (char*) (buffer + argc + 1);

  bufend = estrcpy(cwdbuf, bufend);

  for (int i = 0; i < argc; ++i) {
    buffer[i + 1] = TO_LITTLE_ENDIAN32(bufend - ((char*) buffer));
    bufend = estrcpy(argv[i], bufend);
    if (i == 0 && aDesktopStartupID) {
      bufend = estrcpy(desktopStartupPrefix, bufend - 1);
      bufend = estrcpy(aDesktopStartupID, bufend - 1);
    }
  }

#ifdef DEBUG_bsmedberg
  int32_t   debug_argc   = TO_LITTLE_ENDIAN32(*buffer);
  char *debug_workingdir = (char*) (buffer + argc + 1);

  printf("Sending command line:\n"
         "  working dir: %s\n"
         "  argc:\t%i",
         debug_workingdir,
         debug_argc);

  int32_t  *debug_offset = buffer + 1;
  for (int debug_i = 0; debug_i < debug_argc; ++debug_i)
    printf("  argv[%i]:\t%s\n", debug_i,
           ((char*) buffer) + TO_LITTLE_ENDIAN32(debug_offset[debug_i]));
#endif

  XChangeProperty (mDisplay, aWindow, mMozCommandLineAtom, XA_STRING, 8,
                   PropModeReplace, (unsigned char *) buffer,
                   bufend - ((char*) buffer));
  free(buffer);

  if (!WaitForResponse(aWindow, aResponse, aDestroyed, mMozCommandLineAtom))
    return NS_ERROR_FAILURE;
  
  return NS_OK;
}

bool
XRemoteClient::WaitForResponse(Window aWindow, char **aResponse,
                               bool *aDestroyed, Atom aCommandAtom)
{
  bool done = false;
  bool accepted = false;

  while (!done) {
    XEvent event;
    XNextEvent (mDisplay, &event);
    if (event.xany.type == DestroyNotify &&
        event.xdestroywindow.window == aWindow) {
      
      PR_LOG(sRemoteLm, PR_LOG_DEBUG,
             ("window 0x%x was destroyed.\n",
              (unsigned int) aWindow));
      *aResponse = strdup("Window was destroyed while reading response.");
      *aDestroyed = true;
      return false;
    }
    else if (event.xany.type == PropertyNotify &&
             event.xproperty.state == PropertyNewValue &&
             event.xproperty.window == aWindow &&
             event.xproperty.atom == mMozResponseAtom) {
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytes_after;
      unsigned char *data = 0;
      Bool result;
      result = XGetWindowProperty (mDisplay, aWindow, mMozResponseAtom,
                                   0, (65536 / sizeof (long)),
                                   True, 
                                   XA_STRING,
                                   &actual_type, &actual_format,
                                   &nitems, &bytes_after,
                                   &data);
      if (result != Success) {
        PR_LOG(sRemoteLm, PR_LOG_DEBUG,
               ("failed reading " MOZILLA_RESPONSE_PROP
                " from window 0x%0x.\n",
                (unsigned int) aWindow));
        *aResponse = strdup("Internal error reading response from window.");
        done = true;
      }
      else if (!data || strlen((char *) data) < 5) {
        PR_LOG(sRemoteLm, PR_LOG_DEBUG,
               ("invalid data on " MOZILLA_RESPONSE_PROP
                " property of window 0x%0x.\n",
                (unsigned int) aWindow));
        *aResponse = strdup("Server returned invalid data in response.");
        done = true;
      }
      else if (*data == '1') {  
        PR_LOG(sRemoteLm, PR_LOG_DEBUG,  ("%s\n", data + 4));
        
        done = false;
      }

      else if (!strncmp ((char *)data, "200", 3)) { 
        *aResponse = strdup((char *)data);
        accepted = true;
        done = true;
      }

      else if (*data == '2') {  
        PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("%s\n", data + 4));
        *aResponse = strdup((char *)data);
        accepted = true;
        done = true;
      }

      else if (*data == '3') {  
        PR_LOG(sRemoteLm, PR_LOG_DEBUG,
               ("internal error: "
                "server wants more information?  (%s)\n",
                data));
        *aResponse = strdup((char *)data);
        done = true;
      }

      else if (*data == '4' ||  
               *data == '5') {  
        PR_LOG(sRemoteLm, PR_LOG_DEBUG, ("%s\n", data + 4));
        *aResponse = strdup((char *)data);
        done = true;
      }

      else {
        PR_LOG(sRemoteLm, PR_LOG_DEBUG,
               ("unrecognised " MOZILLA_RESPONSE_PROP
                " from window 0x%x: %s\n",
                (unsigned int) aWindow, data));
        *aResponse = strdup((char *)data);
        done = true;
      }

      if (data)
        XFree(data);
    }

    else if (event.xany.type == PropertyNotify &&
             event.xproperty.window == aWindow &&
             event.xproperty.state == PropertyDelete &&
             event.xproperty.atom == aCommandAtom) {
      PR_LOG(sRemoteLm, PR_LOG_DEBUG,
             ("(server 0x%x has accepted "
              MOZILLA_COMMANDLINE_PROP ".)\n",
              (unsigned int) aWindow));
    }
    
  }

  return accepted;
}

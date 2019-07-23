





































#pragma once

#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserChrome.h"
#include "nsIEmbeddingSiteWindow.h"
#include "nsIBaseWindow.h"
#include "nsIWebNavigation.h"
#include "nsIWindowWatcher.h"
#include "nsIInputStream.h"
#include "nsIURI.h"
#include "nsEmbedAPI.h"
#include "DotNetNetworking.h"
#include "DotNetXPCOM_IO.h"

using namespace System;
using namespace Mozilla::Embedding::Networking;
using namespace Mozilla::Embedding::XPCOM_IO;

namespace Mozilla
{
  namespace Embedding
  {
    namespace WebBrowser
    {
      public __gc class EmbeddingSiteWindow
      {
        private:
          Boolean mVisibility;
          String* mTitle;

        public:
          static const UInt32 DIM_FLAGS_POSITION = 1;
          static const UInt32 DIM_FLAGS_SIZE_INNER = 2;
          static const UInt32 DIM_FLAGS_SIZE_OUTER = 4;

          __property Boolean get_Visibility() { return visibility; };
          __property void set_Visibility(Boolean v) { visibility = v; };
          __property String* get_Title() { return title; };
          __property void set_Title(String * t) { title = t; };

          void setDimensions(UInt32 aFlags, Int32 x, Int32 y, Int32 cx, Int32 cy) {};
          void getDimensions(UInt32 aFlags, Int32 *x, Int32 *y, Int32 *cx, Int32 *cy) {};
          void setFocus() {};
      }; 

      public __gc class WebBrowserChrome
      {
        public:
          bool setStatus() { return true; };
          bool destroyBrowserWindow() { return true; };
          bool sizeBrowserTo() { return true; };
          bool showAsModal() { return true; };
          bool isWindowModal() { return true; };
          bool exitModalEventLoop() { return true; };
      }; 

      public __gc class WebNavigation
      {
        public:
          static const UInt32 LOAD_FLAGS_MASK = 65535;
          static const UInt32 LOAD_FLAGS_NONE = 0;
          static const UInt32 LOAD_FLAGS_IS_REFRESH = 16;
          static const UInt32 LOAD_FLAGS_IS_LINK = 32;
          static const UInt32 LOAD_FLAGS_BYPASS_HISTORY = 64;
          static const UInt32 LOAD_FLAGS_REPLACE_HISTORY = 128;
          static const UInt32 LOAD_FLAGS_BYPASS_CACHE = 256;
          static const UInt32 LOAD_FLAGS_BYPASS_PROXY = 512;
          static const UInt32 LOAD_FLAGS_CHARSET_CHANGE = 1024;
          static const UInt32 STOP_NETWORK = 1;
          static const UInt32 STOP_CONTENT = 2;
          static const UInt32 STOP_ALL = 3;

          void goBack() {};
          void goForward() {};
          void gotoIndex(Int32 arg0) {};
          void loadURI(String *arg0, UInt32 arg1, URI *arg2, InputStream *arg3, InputStream *arg4) {}; 
          void reload(UInt32 arg0) {};
          void stop(UInt32 arg0) {};
      }; 

      public __gc class WebBrowser
      {
        public:
          bool addWebBrowserListener() { return true; };
          bool removeWebBrowserListener() { return true; };
          bool SetContainerWindow(WebBrowserChrome *aChrome) { return true; };
      }; 

      public __gc class WindowWatcher
      {
        public:
          bool openWindow() { return true; };
          bool registerNotification() { return true; };
          bool unregisterNotification() { return true; };
          bool getWindowEnumerator() { return true; };
          bool getNewPrompter() { return true; };
          bool getNewAuthPrompter() { return true; };
          bool setWindowCreator() { return true; };
      }; 

      public __gc class WindowCreator
      {
        public:
          bool createChromeWindow() { return true; };
      }; 

    } 

  } 
}

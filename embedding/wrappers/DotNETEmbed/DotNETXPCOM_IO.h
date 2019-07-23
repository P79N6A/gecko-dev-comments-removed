





































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

using namespace System;

namespace Mozilla
{
  namespace Embedding
  {
    namespace XPCOM_IO
    {
      public __gc class InputStream
      {
        bool Close(void) { return true; };
        bool Available(int *_retval) { return true; };
        bool Read(String * aBuf, int aCount, int *_retval) { return true; };
        bool IsNonBlocking(bool *_retval) { return true; };
      }; 
    }; 
  } 
}

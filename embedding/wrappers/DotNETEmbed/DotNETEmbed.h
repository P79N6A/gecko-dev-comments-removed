





































#include "nsString.h"

#using <mscorlib.dll>
#using <System.Windows.Forms.dll>
#using <System.dll>

using namespace System;
using namespace System::Windows::Forms;

class nsIWebBrowserChrome;

namespace Mozilla
{
  namespace Embedding
  {
    
    

    public __gc class Gecko : public System::Windows::Forms::UserControl
    {
    public:
      Gecko();

      static void TermEmbedding();

      void OpenURL(String *url);

    protected:
      
      void OnResize(EventArgs *e);

    private:
      void CreateBrowserWindow(PRUint32 aChromeFlags,
                               nsIWebBrowserChrome *aParent);

      nsIWebBrowserChrome *mChrome;

      static bool sIsInitialized = false;
    }; 

    
    void ThrowIfFailed(nsresult rv);

    
    
    inline String * CopyString(const nsAFlatString& aStr)
    {
      return new String(aStr.get(), 0, aStr.Length());
    }

    
    
    String * CopyString(const nsAFlatCString& aStr);

    
    
    nsAFlatString& CopyString(String *aSrc, nsAFlatString& aDest);

    
    
    nsAFlatCString& CopyString(String *aSrc, nsAFlatCString& aDest);

  } 
}

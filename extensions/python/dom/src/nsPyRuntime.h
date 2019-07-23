



































#include "nsIScriptRuntime.h"
#include "nsIGenericFactory.h"


#define NS_SCRIPT_LANGUAGE_PYTHON_CID \
  { 0xcee4ee7d, 0xf230, 0x49da, { 0x94, 0xd8, 0x6a, 0x9a, 0x48, 0xe, 0x12, 0xb3 } }

#define NS_SCRIPT_LANGUAGE_PYTHON_CONTRACTID_NAME \
  "@mozilla.org/script-language;1?script-type=application/x-python"


#define NS_SCRIPT_LANGUAGE_PYTHON_CONTRACTID_ID \
  "@mozilla.org/script-language;1?id=3"

class nsPythonRuntime : public nsIScriptRuntime
{
public:
  
  NS_DECL_ISUPPORTS

  
  virtual PRUint32 GetScriptTypeID() {
    return nsIProgrammingLanguage::PYTHON;
  }

  virtual void ShutDown() {;}

  virtual nsresult CreateContext(nsIScriptContext **ret);

  virtual nsresult ParseVersion(const nsString &aVersionStr, PRUint32 *flags);

  virtual nsresult HoldScriptObject(void *object);
  virtual nsresult DropScriptObject(void *object);
 
};

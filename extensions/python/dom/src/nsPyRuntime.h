



































#include "nsIScriptRuntime.h"
#include "nsIGenericFactory.h"


#define NS_SCRIPT_LANGUAGE_PYTHON_CID \
  { 0x1f8c24f2, 0x8808, 0x47ec, { 0x81, 0x12, 0x94, 0x99, 0xb3, 0x70, 0x1b, 0x68 } }

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

  virtual nsresult CreateContext(nsIScriptContext **ret);

  virtual nsresult ParseVersion(const nsString &aVersionStr, PRUint32 *flags);

  virtual nsresult HoldScriptObject(void *object);
  virtual nsresult DropScriptObject(void *object);
 
};

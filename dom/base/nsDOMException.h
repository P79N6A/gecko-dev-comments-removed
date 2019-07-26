





#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif

#include "mozilla/NullPtr.h"
#include "nsError.h"
#include "nsIDOMDOMException.h"
#include "nsIException.h"
#include "nsCOMPtr.h"
#include "xpcprivate.h"

template<typename> class already_AddRefed;

class nsDOMException : public nsXPCException,
                       public nsIDOMDOMException
{
public:
  nsDOMException(nsresult aRv, const char* aMessage,
                 const char* aName, uint16_t aCode);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDOMEXCEPTION

  
  NS_IMETHOD ToString(char **aReturn) MOZ_OVERRIDE;

  
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
    MOZ_OVERRIDE;

  uint16_t Code() const {
    return mCode;
  }

  
  void GetMessageMoz(nsString& retval);
  void GetName(nsString& retval);

protected:

  virtual ~nsDOMException() {}

  
  const char* mName;
  const char* mMessage;

  uint16_t mCode;
};

nsresult
NS_GetNameAndMessageForDOMNSResult(nsresult aNSResult, const char** aName,
                                   const char** aMessage,
                                   uint16_t* aCode = nullptr);

already_AddRefed<nsDOMException>
NS_NewDOMException(nsresult aNSResult);

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

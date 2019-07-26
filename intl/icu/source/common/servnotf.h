





#ifndef ICUNOTIF_H
#define ICUNOTIF_H

#include "unicode/utypes.h"

#if UCONFIG_NO_SERVICE

U_NAMESPACE_BEGIN





class ICUNotifier;

U_NAMESPACE_END

#else

#include "unicode/uobject.h"
#include "unicode/unistr.h"

#include "mutex.h"
#include "uvector.h"

U_NAMESPACE_BEGIN

class U_COMMON_API EventListener : public UObject {
public: 
    virtual ~EventListener();

public:
    static UClassID U_EXPORT2 getStaticClassID();

    virtual UClassID getDynamicClassID() const;

public:
#ifdef SERVICE_DEBUG
    virtual UnicodeString& debug(UnicodeString& result) const {
      return debugClass(result);
    }

    virtual UnicodeString& debugClass(UnicodeString& result) const {
      return result.append("Key");
    }
#endif
};


















class U_COMMON_API ICUNotifier : public UMemory  {
private: UVector* listeners;
         
public: 
    ICUNotifier(void);
    
    virtual ~ICUNotifier(void);
    
    






    virtual void addListener(const EventListener* l, UErrorCode& status);
    
    




    virtual void removeListener(const EventListener* l, UErrorCode& status);
    
    





    virtual void notifyChanged(void);
    
protected: 
    



    virtual UBool acceptsListener(const EventListener& l) const = 0;
    
    


    virtual void notifyListener(EventListener& l) const = 0;
};

U_NAMESPACE_END


#endif


#endif

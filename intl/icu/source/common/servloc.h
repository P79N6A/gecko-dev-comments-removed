







#ifndef ICULSERV_H
#define ICULSERV_H

#include "unicode/utypes.h"

#if UCONFIG_NO_SERVICE

U_NAMESPACE_BEGIN





class ICULocaleService;

U_NAMESPACE_END

#else

#include "unicode/unistr.h"
#include "unicode/locid.h"
#include "unicode/strenum.h"

#include "hash.h"
#include "uvector.h"

#include "serv.h"
#include "locutil.h"

U_NAMESPACE_BEGIN

class ICULocaleService;

class LocaleKey;
class LocaleKeyFactory;
class SimpleLocaleKeyFactory;
class ServiceListener;


















class U_COMMON_API LocaleKey : public ICUServiceKey {
  private: 
    int32_t _kind;
    UnicodeString _primaryID;
    UnicodeString _fallbackID;
    UnicodeString _currentID;

  public:
    enum {
        KIND_ANY = -1
    };

    


    static LocaleKey* createWithCanonicalFallback(const UnicodeString* primaryID, 
                                                  const UnicodeString* canonicalFallbackID,
                                                  UErrorCode& status);

    


    static LocaleKey* createWithCanonicalFallback(const UnicodeString* primaryID, 
                                                  const UnicodeString* canonicalFallbackID, 
                                                  int32_t kind,
                                                  UErrorCode& status);

  protected:
    





    LocaleKey(const UnicodeString& primaryID, 
              const UnicodeString& canonicalPrimaryID, 
              const UnicodeString* canonicalFallbackID, 
              int32_t kind);

 public:
    


    virtual UnicodeString& prefix(UnicodeString& result) const;

    


    virtual int32_t kind() const;

    


    virtual UnicodeString& canonicalID(UnicodeString& result) const;

    


    virtual UnicodeString& currentID(UnicodeString& result) const;

    


    virtual UnicodeString& currentDescriptor(UnicodeString& result) const;

    


    virtual Locale& canonicalLocale(Locale& result) const;

    


    virtual Locale& currentLocale(Locale& result) const;

    








    virtual UBool fallback();

    



    virtual UBool isFallbackOf(const UnicodeString& id) const;
    
 public:
    


    static UClassID U_EXPORT2 getStaticClassID();

    virtual UClassID getDynamicClassID() const;

    


    virtual ~LocaleKey();

#ifdef SERVICE_DEBUG
 public:
    virtual UnicodeString& debug(UnicodeString& result) const;
    virtual UnicodeString& debugClass(UnicodeString& result) const;
#endif

};



















class U_COMMON_API LocaleKeyFactory : public ICUServiceFactory {
protected:
    const UnicodeString _name;
    const int32_t _coverage;

public:
    enum {
        




        VISIBLE = 0,

        




        INVISIBLE = 1
    };

    


    virtual ~LocaleKeyFactory();

protected:
    


    LocaleKeyFactory(int32_t coverage);

    


    LocaleKeyFactory(int32_t coverage, const UnicodeString& name);

    




public:
    virtual UObject* create(const ICUServiceKey& key, const ICUService* service, UErrorCode& status) const;

protected:
    virtual UBool handlesKey(const ICUServiceKey& key, UErrorCode& status) const;

public:
    



    virtual void updateVisibleIDs(Hashtable& result, UErrorCode& status) const;

    


    virtual UnicodeString& getDisplayName(const UnicodeString& id, const Locale& locale, UnicodeString& result) const;

protected:
    



    virtual UObject* handleCreate(const Locale& loc, int32_t kind, const ICUService* service, UErrorCode& status) const;

   



 

   




    virtual const Hashtable* getSupportedIDs(UErrorCode& status) const;

public:
    


    static UClassID U_EXPORT2 getStaticClassID();

    virtual UClassID getDynamicClassID() const;

#ifdef SERVICE_DEBUG
 public:
    virtual UnicodeString& debug(UnicodeString& result) const;
    virtual UnicodeString& debugClass(UnicodeString& result) const;
#endif

};









class U_COMMON_API SimpleLocaleKeyFactory : public LocaleKeyFactory {
 private:
    UObject* _obj;
    UnicodeString _id;
    const int32_t _kind;

 public:
    SimpleLocaleKeyFactory(UObject* objToAdopt, 
                           const UnicodeString& locale, 
                           int32_t kind, 
                           int32_t coverage);

    SimpleLocaleKeyFactory(UObject* objToAdopt, 
                           const Locale& locale, 
                           int32_t kind, 
                           int32_t coverage);

    


    virtual ~SimpleLocaleKeyFactory();

    


    virtual UObject* create(const ICUServiceKey& key, const ICUService* service, UErrorCode& status) const;

    



    virtual void updateVisibleIDs(Hashtable& result, UErrorCode& status) const;

 protected:
    


    


public:
    


    static UClassID U_EXPORT2 getStaticClassID();

    virtual UClassID getDynamicClassID() const;

#ifdef SERVICE_DEBUG
 public:
    virtual UnicodeString& debug(UnicodeString& result) const;
    virtual UnicodeString& debugClass(UnicodeString& result) const;
#endif

};












class U_COMMON_API ICUResourceBundleFactory : public LocaleKeyFactory 
{
 protected:
    UnicodeString _bundleName;

 public:
    


    ICUResourceBundleFactory();

    





    ICUResourceBundleFactory(const UnicodeString& bundleName);

    


    virtual ~ICUResourceBundleFactory();

protected:
    


    virtual const Hashtable* getSupportedIDs(UErrorCode& status) const;

    



    virtual UObject* handleCreate(const Locale& loc, int32_t kind, const ICUService* service, UErrorCode& status) const;

public:
    


    static UClassID U_EXPORT2 getStaticClassID();
    virtual UClassID getDynamicClassID() const;


#ifdef SERVICE_DEBUG
 public:
    virtual UnicodeString& debug(UnicodeString& result) const;
    virtual UnicodeString& debugClass(UnicodeString& result) const;
#endif

};





class U_COMMON_API ICULocaleService : public ICUService 
{
 private:
  Locale fallbackLocale;
  UnicodeString fallbackLocaleName;

 public:
  


  ICULocaleService();

  


  ICULocaleService(const UnicodeString& name);

  


  virtual ~ICULocaleService();

#if 0
  
  
  
  UObject* get(const UnicodeString& descriptor, UErrorCode& status) const {
    return ICUService::get(descriptor, status);
  }

  UObject* get(const UnicodeString& descriptor, UnicodeString* actualReturn, UErrorCode& status) const {
    return ICUService::get(descriptor, actualReturn, status);
  }
#endif

  




  UObject* get(const Locale& locale, UErrorCode& status) const;

  



  UObject* get(const Locale& locale, int32_t kind, UErrorCode& status) const;

  



  UObject* get(const Locale& locale, Locale* actualReturn, UErrorCode& status) const;
                   
  





  UObject* get(const Locale& locale, int32_t kind, Locale* actualReturn, UErrorCode& status) const;

  




  virtual URegistryKey registerInstance(UObject* objToAdopt, const Locale& locale, UErrorCode& status);

  




  virtual URegistryKey registerInstance(UObject* objToAdopt, const Locale& locale, int32_t kind, UErrorCode& status);

  



  virtual URegistryKey registerInstance(UObject* objToAdopt, const Locale& locale, int32_t kind, int32_t coverage, UErrorCode& status);


  






  virtual URegistryKey registerInstance(UObject* objToAdopt, const UnicodeString& locale, UBool visible, UErrorCode& status);

  



  virtual StringEnumeration* getAvailableLocales(void) const;

 protected:

  



  const UnicodeString& validateFallbackLocale() const;

  


  virtual ICUServiceKey* createKey(const UnicodeString* id, UErrorCode& status) const;

  


  virtual ICUServiceKey* createKey(const UnicodeString* id, int32_t kind, UErrorCode& status) const;

  friend class ServiceEnumeration;
};

U_NAMESPACE_END

    
#endif


#endif


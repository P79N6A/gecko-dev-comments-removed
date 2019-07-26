






#ifndef ICUSERV_H
#define ICUSERV_H

#include "unicode/utypes.h"

#if UCONFIG_NO_SERVICE

U_NAMESPACE_BEGIN





class ICUService;

U_NAMESPACE_END

#else

#include "unicode/unistr.h"
#include "unicode/locid.h"
#include "unicode/umisc.h"

#include "hash.h"
#include "uvector.h"
#include "servnotf.h"

class ICUServiceTest;

U_NAMESPACE_BEGIN

class ICUServiceKey;
class ICUServiceFactory;
class SimpleFactory;
class ServiceListener;
class ICUService;

class DNCache;
























class U_COMMON_API ICUServiceKey : public UObject {
 private: 
  const UnicodeString _id;

 protected:
  static const UChar PREFIX_DELIMITER;

 public:

  




  ICUServiceKey(const UnicodeString& id);

  


  virtual ~ICUServiceKey();

 




  virtual const UnicodeString& getID() const;

 






  virtual UnicodeString& canonicalID(UnicodeString& result) const;

 






  virtual UnicodeString& currentID(UnicodeString& result) const;

 













  virtual UnicodeString& currentDescriptor(UnicodeString& result) const;

 








  virtual UBool fallback();

 






  virtual UBool isFallbackOf(const UnicodeString& id) const;

 






  virtual UnicodeString& prefix(UnicodeString& result) const;

 








  static UnicodeString& parsePrefix(UnicodeString& result);

  








  static UnicodeString& parseSuffix(UnicodeString& result);

public:
  


  static UClassID U_EXPORT2 getStaticClassID();

  


  virtual UClassID getDynamicClassID() const;

#ifdef SERVICE_DEBUG
 public:
  virtual UnicodeString& debug(UnicodeString& result) const;
  virtual UnicodeString& debugClass(UnicodeString& result) const;
#endif

};

 



 





class U_COMMON_API ICUServiceFactory : public UObject {
 public:
    virtual ~ICUServiceFactory();

    
















    virtual UObject* create(const ICUServiceKey& key, const ICUService* service, UErrorCode& status) const = 0;

    















    virtual void updateVisibleIDs(Hashtable& result, UErrorCode& status) const = 0;

    












    virtual UnicodeString& getDisplayName(const UnicodeString& id, const Locale& locale, UnicodeString& result) const = 0;
};





 









class U_COMMON_API SimpleFactory : public ICUServiceFactory {
 protected:
  UObject* _instance;
  const UnicodeString _id;
  const UBool _visible;

 public:
  









  SimpleFactory(UObject* instanceToAdopt, const UnicodeString& id, UBool visible = TRUE);

  


  virtual ~SimpleFactory();

  








  virtual UObject* create(const ICUServiceKey& key, const ICUService* service, UErrorCode& status) const;

  






  virtual void updateVisibleIDs(Hashtable& result, UErrorCode& status) const;

  









  virtual UnicodeString& getDisplayName(const UnicodeString& id, const Locale& locale, UnicodeString& result) const;

public:
 


  static UClassID U_EXPORT2 getStaticClassID();

 


  virtual UClassID getDynamicClassID() const;

#ifdef SERVICE_DEBUG
 public:
  virtual UnicodeString& debug(UnicodeString& toAppendTo) const;
  virtual UnicodeString& debugClass(UnicodeString& toAppendTo) const;
#endif

};












class U_COMMON_API ServiceListener : public EventListener {
public:
    virtual ~ServiceListener();

    






    virtual void serviceChanged(const ICUService& service) const = 0;
    
public:
    


    static UClassID U_EXPORT2 getStaticClassID();
    
    


    virtual UClassID getDynamicClassID() const;
    
};









class U_COMMON_API StringPair : public UMemory {
public:
  


  const UnicodeString displayName;

  


  const UnicodeString id;

  







  static StringPair* create(const UnicodeString& displayName, 
                            const UnicodeString& id,
                            UErrorCode& status);

  



  UBool isBogus() const;

private:
  StringPair(const UnicodeString& displayName, const UnicodeString& id);
};





 


















































































class U_COMMON_API ICUService : public ICUNotifier {
 protected: 
    


    const UnicodeString name;

 private:

    


    uint32_t timestamp;

    


    UVector* factories;

    


    Hashtable* serviceCache;

    


    Hashtable* idCache;

    


    DNCache* dnCache;

    


 public:
    


    ICUService();

    




    ICUService(const UnicodeString& name);

    


    virtual ~ICUService();

    






    UnicodeString& getName(UnicodeString& result) const;

    







    UObject* get(const UnicodeString& descriptor, UErrorCode& status) const;

    








    UObject* get(const UnicodeString& descriptor, UnicodeString* actualReturn, UErrorCode& status) const;

    






    UObject* getKey(ICUServiceKey& key, UErrorCode& status) const;

    





















    virtual UObject* getKey(ICUServiceKey& key, UnicodeString* actualReturn, UErrorCode& status) const;

    











    UObject* getKey(ICUServiceKey& key, UnicodeString* actualReturn, const ICUServiceFactory* factory, UErrorCode& status) const;

    







    UVector& getVisibleIDs(UVector& result, UErrorCode& status) const;

    



















    UVector& getVisibleIDs(UVector& result, const UnicodeString* matchID, UErrorCode& status) const;

    







    UnicodeString& getDisplayName(const UnicodeString& id, UnicodeString& result) const;

    









    UnicodeString& getDisplayName(const UnicodeString& id, UnicodeString& result, const Locale& locale) const;

    








    UVector& getDisplayNames(UVector& result, UErrorCode& status) const;

    








    UVector& getDisplayNames(UVector& result, const Locale& locale, UErrorCode& status) const;

    






















    UVector& getDisplayNames(UVector& result,
                             const Locale& locale, 
                             const UnicodeString* matchID, 
                             UErrorCode& status) const;

    









    URegistryKey registerInstance(UObject* objToAdopt, const UnicodeString& id, UErrorCode& status);

    

















    virtual URegistryKey registerInstance(UObject* objToAdopt, const UnicodeString& id, UBool visible, UErrorCode& status);

    















    virtual URegistryKey registerFactory(ICUServiceFactory* factoryToAdopt, UErrorCode& status);

    












    virtual UBool unregister(URegistryKey rkey, UErrorCode& status);

    





    virtual void reset(void);

    





    virtual UBool isDefault(void) const;

    










    virtual ICUServiceKey* createKey(const UnicodeString* id, UErrorCode& status) const;

    







    virtual UObject* cloneInstance(UObject* instance) const = 0;


    



 protected:

    










    virtual ICUServiceFactory* createSimpleFactory(UObject* instanceToAdopt, const UnicodeString& id, UBool visible, UErrorCode& status);

    











    virtual void reInitializeFactories(void);

    










    virtual UObject* handleDefault(const ICUServiceKey& key, UnicodeString* actualReturn, UErrorCode& status) const;

    







    virtual void clearCaches(void);

    









    virtual UBool acceptsListener(const EventListener& l) const;

    








    virtual void notifyListener(EventListener& l) const;

    



    







    void clearServiceCache(void);

    







    const Hashtable* getVisibleIDMap(UErrorCode& status) const;

    




    int32_t getTimestamp(void) const;

    




    int32_t countFactories(void) const;

private:

    friend class ::ICUServiceTest; 
};

U_NAMESPACE_END

    
#endif

    
#endif


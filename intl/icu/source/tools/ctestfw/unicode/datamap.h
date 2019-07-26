







#ifndef U_TESTFW_DATAMAP
#define U_TESTFW_DATAMAP

#include "unicode/resbund.h"
#include "unicode/testtype.h"



U_NAMESPACE_BEGIN
class Hashtable;
U_NAMESPACE_END







class T_CTEST_EXPORT_API DataMap {
public:
  virtual ~DataMap();

protected:
  DataMap();
  int32_t utoi(const UnicodeString &s) const;


public:
  



  virtual const UnicodeString getString(const char* key, UErrorCode &status) const = 0;

  




  virtual int32_t getInt(const char* key, UErrorCode &status) const = 0;

  





  virtual int32_t getInt28(const char* key, UErrorCode &status) const = 0;

  





  virtual uint32_t getUInt28(const char* key, UErrorCode &status) const = 0;

  






  virtual const int32_t *getIntVector(int32_t &length, const char *key, UErrorCode &status) const = 0;

  






  virtual const uint8_t *getBinary(int32_t &length, const char *key, UErrorCode &status) const = 0;

  




  virtual const UnicodeString* getStringArray(int32_t& count, const char* key, UErrorCode &status) const = 0;

  




  virtual const int32_t* getIntArray(int32_t& count, const char* key, UErrorCode &status) const = 0;

  
};



class T_CTEST_EXPORT_API RBDataMap : public DataMap{
private:
  Hashtable *fData;

public:
  virtual ~RBDataMap();

public:
  RBDataMap();

  RBDataMap(UResourceBundle *data, UErrorCode &status);
  RBDataMap(UResourceBundle *headers, UResourceBundle *data, UErrorCode &status);

public:
  void init(UResourceBundle *data, UErrorCode &status);
  void init(UResourceBundle *headers, UResourceBundle *data, UErrorCode &status);

  virtual const ResourceBundle *getItem(const char* key, UErrorCode &status) const;

  virtual const UnicodeString getString(const char* key, UErrorCode &status) const;
  virtual int32_t getInt28(const char* key, UErrorCode &status) const;
  virtual uint32_t getUInt28(const char* key, UErrorCode &status) const;
  virtual const int32_t *getIntVector(int32_t &length, const char *key, UErrorCode &status) const;
  virtual const uint8_t *getBinary(int32_t &length, const char *key, UErrorCode &status) const;

  virtual int32_t getInt(const char* key, UErrorCode &status) const;
  
  virtual const UnicodeString* getStringArray(int32_t& count, const char* key, UErrorCode &status) const;
  virtual const int32_t* getIntArray(int32_t& count, const char* key, UErrorCode &status) const;

  
};


#endif






#ifndef _INC_NSSCleaner_H
#define _INC_NSSCleaner_H















































#define NSSCleanupAutoPtrClass(nsstype, cleanfunc) \
class nsstype##Cleaner                             \
{                                                  \
private:                                           \
  nsstype##Cleaner(const nsstype##Cleaner&);       \
  nsstype##Cleaner();                              \
  void operator=(const nsstype##Cleaner&);         \
  nsstype *&object;                                \
public:                                            \
  explicit nsstype##Cleaner(nsstype *&a_object)    \
    :object(a_object) {}                           \
  ~nsstype##Cleaner() {                            \
    if (object) {                                  \
      cleanfunc(object);                           \
      object = nullptr;                             \
    }                                              \
  }                                                \
  void detach() {object=nullptr;}                   \
};

#define NSSCleanupAutoPtrClass_WithParam(nsstype, cleanfunc, namesuffix, paramvalue) \
class nsstype##Cleaner##namesuffix                 \
{                                                  \
private:                                           \
  nsstype##Cleaner##namesuffix(const nsstype##Cleaner##namesuffix &); \
  nsstype##Cleaner##namesuffix();                                     \
  void operator=(const nsstype##Cleaner##namesuffix &);               \
  nsstype *&object;                                \
public:                                            \
  explicit nsstype##Cleaner##namesuffix(nsstype *&a_object)           \
    :object(a_object) {}                           \
  ~nsstype##Cleaner##namesuffix() {                \
    if (object) {                                  \
      cleanfunc(object, paramvalue);               \
      object = nullptr;                             \
    }                                              \
  }                                                \
  void detach() {object=nullptr;}                   \
};

#include "certt.h"

class CERTVerifyLogContentsCleaner
{
public:
  explicit CERTVerifyLogContentsCleaner(CERTVerifyLog *&cvl);
  ~CERTVerifyLogContentsCleaner();
private:
  CERTVerifyLog *&m_cvl;
};

#endif








#ifndef FPHDLIMP_H
#define FPHDLIMP_H

#if !UCONFIG_NO_FORMATTING

#include "unicode/utypes.h"
#include "unicode/fieldpos.h"
#include "unicode/fpositer.h"

U_NAMESPACE_BEGIN




class FieldPositionHandler: public UMemory {
 public:
  virtual ~FieldPositionHandler();
  virtual void addAttribute(int32_t id, int32_t start, int32_t limit);
  virtual void shiftLast(int32_t delta);
  virtual UBool isRecording(void);
};




class FieldPositionOnlyHandler : public FieldPositionHandler {
  FieldPosition& pos;

 public:
  FieldPositionOnlyHandler(FieldPosition& pos);
  virtual ~FieldPositionOnlyHandler();

  virtual void addAttribute(int32_t id, int32_t start, int32_t limit);
  virtual void shiftLast(int32_t delta);
  virtual UBool isRecording(void);
};




class FieldPositionIteratorHandler : public FieldPositionHandler {
  FieldPositionIterator* iter; 
  UVector32* vec;
  UErrorCode status;

  
  
  
  
  void *operator new(size_t s);
  void *operator new[](size_t s);

 public:
  FieldPositionIteratorHandler(FieldPositionIterator* posIter, UErrorCode& status);
  ~FieldPositionIteratorHandler();

  virtual void addAttribute(int32_t id, int32_t start, int32_t limit);
  virtual void shiftLast(int32_t delta);
  virtual UBool isRecording(void);
};

U_NAMESPACE_END

#endif 

#endif 

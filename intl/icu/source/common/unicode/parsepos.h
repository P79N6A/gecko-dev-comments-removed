














#ifndef PARSEPOS_H
#define PARSEPOS_H

#include "unicode/utypes.h"
#include "unicode/uobject.h"

 
U_NAMESPACE_BEGIN























class U_COMMON_API ParsePosition : public UObject {
public:
    



    ParsePosition()
        : UObject(),
        index(0),
        errorIndex(-1)
      {}

    




    ParsePosition(int32_t newIndex)
        : UObject(),
        index(newIndex),
        errorIndex(-1)
      {}

    




    ParsePosition(const ParsePosition& copy)
        : UObject(copy),
        index(copy.index),
        errorIndex(copy.errorIndex)
      {}

    



    virtual ~ParsePosition();

    



    ParsePosition&      operator=(const ParsePosition& copy);

    




    UBool              operator==(const ParsePosition& that) const;

    




    UBool              operator!=(const ParsePosition& that) const;

    










    ParsePosition *clone() const;

    






    int32_t getIndex(void) const;

    




    void setIndex(int32_t index);

    






    void setErrorIndex(int32_t ei);

    




    int32_t getErrorIndex(void) const;

    




    static UClassID U_EXPORT2 getStaticClassID();

    




    virtual UClassID getDynamicClassID() const;

private:
    





    int32_t index;

    


    int32_t errorIndex;

};

inline ParsePosition&
ParsePosition::operator=(const ParsePosition& copy)
{
  index = copy.index;
  errorIndex = copy.errorIndex;
  return *this;
}

inline UBool
ParsePosition::operator==(const ParsePosition& copy) const
{
  if(index != copy.index || errorIndex != copy.errorIndex)
  return FALSE;
  else
  return TRUE;
}

inline UBool
ParsePosition::operator!=(const ParsePosition& copy) const
{
  return !operator==(copy);
}

inline int32_t
ParsePosition::getIndex() const
{
  return index;
}

inline void
ParsePosition::setIndex(int32_t offset)
{
  this->index = offset;
}

inline int32_t
ParsePosition::getErrorIndex() const
{
  return errorIndex;
}

inline void
ParsePosition::setErrorIndex(int32_t ei)
{
  this->errorIndex = ei;
}
U_NAMESPACE_END

#endif

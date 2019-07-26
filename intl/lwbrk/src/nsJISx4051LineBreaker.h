



#ifndef nsJISx4051LineBreaker_h__
#define nsJISx4051LineBreaker_h__


#include "nsILineBreaker.h"

class nsJISx4051LineBreaker : public nsILineBreaker
{
  NS_DECL_ISUPPORTS

private:
  virtual ~nsJISx4051LineBreaker();

public:
  nsJISx4051LineBreaker();

  int32_t Next( const char16_t* aText, uint32_t aLen, uint32_t aPos);

  int32_t Prev( const char16_t* aText, uint32_t aLen, uint32_t aPos);

  virtual void GetJISx4051Breaks(const char16_t* aText, uint32_t aLength,
                                 uint8_t aBreakMode,
                                 uint8_t* aBreakBefore);
  virtual void GetJISx4051Breaks(const uint8_t* aText, uint32_t aLength,
                                 uint8_t aBreakMode,
                                 uint8_t* aBreakBefore);

private:
  int32_t WordMove(const char16_t* aText, uint32_t aLen, uint32_t aPos,
                   int8_t aDirection);
};

#endif  

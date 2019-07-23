



































#ifndef nsJISx4501LineBreaker_h__
#define nsJISx4501LineBreaker_h__


#include "nsILineBreaker.h"

class nsJISx4051LineBreaker : public nsILineBreaker
{
  NS_DECL_ISUPPORTS

public:
  nsJISx4051LineBreaker();
  virtual ~nsJISx4051LineBreaker();

  PRInt32 Next( const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos);

  PRInt32 Prev( const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos);

  virtual void GetJISx4051Breaks(const PRUnichar* aText, PRUint32 aLength,
                                 PRPackedBool* aBreakBefore);
  virtual void GetJISx4051Breaks(const PRUint8* aText, PRUint32 aLength,
                                 PRPackedBool* aBreakBefore);

private:
  PRInt32 WordMove(const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos,
                   PRInt8 aDirection);
};

#endif  

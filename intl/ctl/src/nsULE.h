






































#ifndef nsULE_H
#define nsULE_H

#include "nscore.h"
#include "prtypes.h"

#include "nsCtlCIID.h"
#include "nsILE.h"

#include "pango-types.h"
#include "pango-glyph.h"


class nsULE : public nsILE {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ULE_IID)
  NS_DEFINE_STATIC_CID_ACCESSOR(NS_ULE_CID)
  NS_DECL_ISUPPORTS

  nsULE(void);
  virtual ~nsULE(void);

  
  
  
  

  NS_IMETHOD NeedsCTLFix(const PRUnichar         *aString,
                         const PRInt32           aBeg,
                         const PRInt32           aEnd,
                         PRBool                  *aCTLNeeded);

  NS_IMETHOD GetPresentationForm(const PRUnichar *aString,
                                 PRUint32        aLength,
                                 const char      *aFontCharset,
                                 char            *aGlyphs,
                                 PRSize          *aOutLength,
                                 PRBool          aIsWide = PR_FALSE);

  NS_IMETHOD PrevCluster(const PRUnichar         *aString,
                         PRUint32                aLength,
                         const PRInt32           aIndex,
                         PRInt32                 *aPrevOffset);

  NS_IMETHOD NextCluster(const PRUnichar         *aString,
                         PRUint32                aLength,
                         const PRInt32           aIndex,
                         PRInt32                 *aNextOffset);

  NS_IMETHOD GetRangeOfCluster(const PRUnichar   *aString,
                               PRUint32          aLength,
                               const PRInt32     aIndex,
                               PRInt32           *aStart,
                               PRInt32           *aEnd);

 private:
  const char* GetDefaultFont(const PRUnichar);
  PRInt32     GetGlyphInfo(const PRUnichar*, PRInt32,
                           PangoliteGlyphString*,
                           const char* = (const char*)NULL);
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsULE, NS_ULE_IID)

#endif 







































#ifndef nsStaticAtom_h__
#define nsStaticAtom_h__

#include "nsIAtom.h"











struct nsStaticAtom {
    const char* mString;
    nsIAtom ** mAtom;
};

class nsStaticAtomWrapper : public nsIAtom
{
public:
  nsStaticAtomWrapper(const nsStaticAtom* aAtom, PRUint32 aLength) :
    mStaticAtom(aAtom), mLength(aLength)
  {
    MOZ_COUNT_CTOR(nsStaticAtomWrapper);
  }
  ~nsStaticAtomWrapper() {   
    
    
    
    MOZ_COUNT_DTOR(nsStaticAtomWrapper);
  }

  NS_IMETHOD QueryInterface(REFNSIID aIID,
                            void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  NS_DECL_NSIATOM

  const nsStaticAtom* GetStaticAtom() const {
    return mStaticAtom;
  }

  PRUint32 getLength() const {
    return mLength;
  }

private:
  const nsStaticAtom* mStaticAtom;

  
  
  
  PRUint32 mLength;
};



NS_COM nsresult
NS_RegisterStaticAtoms(const nsStaticAtom*, PRUint32 aAtomCount);

#endif

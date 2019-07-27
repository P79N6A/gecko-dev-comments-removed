





#include "unicode/translit.h"
#include "unicode/normlzr.h"

class UnaccentTransliterator : public Transliterator {
    
 public:
    
    


    UnaccentTransliterator();

    


    virtual ~UnaccentTransliterator();

 protected:

    


    virtual void handleTransliterate(Replaceable& text,
                                     UTransPosition& index,
                                     UBool incremental) const;

 private:

    


    UChar unaccent(UChar c) const;

    Normalizer normalizer;

public:

    










    static inline UClassID getStaticClassID(void) { return (UClassID)&fgClassID; };

    






















    virtual UClassID getDynamicClassID(void) const { return getStaticClassID(); };

private:

    



    static const char fgClassID;
};

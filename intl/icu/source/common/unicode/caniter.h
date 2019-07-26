






#ifndef CANITER_H
#define CANITER_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#include "unicode/uobject.h"
#include "unicode/unistr.h"





 




#ifndef CANITER_SKIP_ZEROES
#define CANITER_SKIP_ZEROES TRUE
#endif

U_NAMESPACE_BEGIN

class Hashtable;
class Normalizer2;
class Normalizer2Impl;




































class U_COMMON_API CanonicalIterator : public UObject {
public:
    





    CanonicalIterator(const UnicodeString &source, UErrorCode &status);

    



    virtual ~CanonicalIterator();

    




    UnicodeString getSource();

    



    void reset();

    






    UnicodeString next();

    






    void setSource(const UnicodeString &newSource, UErrorCode &status);

#ifndef U_HIDE_INTERNAL_API
    








    static void U_EXPORT2 permute(UnicodeString &source, UBool skipZeros, Hashtable *result, UErrorCode &status);
#endif  

    




    static UClassID U_EXPORT2 getStaticClassID();

    




    virtual UClassID getDynamicClassID() const;

private:
    
    
    CanonicalIterator();


    



    CanonicalIterator(const CanonicalIterator& other);

    



    CanonicalIterator& operator=(const CanonicalIterator& other);

    
    UnicodeString source;
    UBool done;

    
    
    UnicodeString **pieces;
    int32_t pieces_length;
    int32_t *pieces_lengths;

    
    int32_t *current;
    int32_t current_length;

    
    UnicodeString buffer;

    const Normalizer2 &nfd;
    const Normalizer2Impl &nfcImpl;

    
    UnicodeString *getEquivalents(const UnicodeString &segment, int32_t &result_len, UErrorCode &status); 

    
    Hashtable *getEquivalents2(Hashtable *fillinResult, const UChar *segment, int32_t segLen, UErrorCode &status);
    

    




    
    Hashtable *extract(Hashtable *fillinResult, UChar32 comp, const UChar *segment, int32_t segLen, int32_t segmentPos, UErrorCode &status);
    

    void cleanPieces();

};

U_NAMESPACE_END

#endif 

#endif

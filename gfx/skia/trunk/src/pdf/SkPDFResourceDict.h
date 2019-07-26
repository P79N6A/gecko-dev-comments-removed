






#ifndef SkPDFResourceDict_DEFINED
#define SkPDFResourceDict_DEFINED

#include "SkPDFTypes.h"
#include "SkTDArray.h"
#include "SkTSet.h"
#include "SkTypes.h"







class SkPDFResourceDict : public SkPDFDict {
public:
    SK_DECLARE_INST_COUNT(SkPDFResourceDict)

     enum SkPDFResourceType{
        kExtGState_ResourceType,
        kPattern_ResourceType,
        kXObject_ResourceType,
        kFont_ResourceType,
        
        
        kResourceTypeCount
     };

    



    SkPDFResourceDict();

    











    SkPDFObject* insertResourceAsReference(SkPDFResourceType type, int key,
                                           SkPDFObject* value);

    








    void getReferencedResources(
            const SkTSet<SkPDFObject*>& knownResourceObjects,
            SkTSet<SkPDFObject*>* newResourceObjects,
            bool recursive) const;

    







    static SkString getResourceName(SkPDFResourceType type, int key);

private:
    









    SkPDFObject* insertResource(SkPDFResourceType type, int key,
                                SkPDFObject* value);

    SkTSet<SkPDFObject*> fResources;

    SkTDArray<SkPDFDict*> fTypes;
    typedef SkPDFDict INHERITED;
};

#endif

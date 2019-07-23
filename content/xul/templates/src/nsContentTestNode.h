





































#ifndef nsContentTestNode_h__
#define nsContentTestNode_h__

#include "nscore.h"
#include "nsRuleNetwork.h"
#include "nsFixedSizeAllocator.h"
#include "nsIAtom.h"
#include "nsIDOMDocument.h"

class nsIXULTemplateBuilder;






class nsContentTestNode : public TestNode
{
public:
    nsContentTestNode(nsXULTemplateQueryProcessorRDF* aProcessor,
                      nsIAtom* aContentVariable);

    virtual nsresult FilterInstantiations(InstantiationSet& aInstantiations,
                                          PRBool* aCantHandleYet) const;

    nsresult
    Constrain(InstantiationSet& aInstantiations);

    void SetTag(nsIAtom* aTag, nsIDOMDocument* aDocument)
    {
        mTag = aTag;
        mDocument = aDocument;
    }

protected:
    nsXULTemplateQueryProcessorRDF *mProcessor;
    nsIDOMDocument* mDocument;
    nsCOMPtr<nsIAtom> mRefVariable;
    nsCOMPtr<nsIAtom> mTag;
};

#endif 


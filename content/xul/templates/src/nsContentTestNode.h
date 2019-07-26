




#ifndef nsContentTestNode_h__
#define nsContentTestNode_h__

#include "mozilla/Attributes.h"
#include "nscore.h"
#include "nsRuleNetwork.h"
#include "nsIAtom.h"
#include "nsIDOMDocument.h"

class nsIXULTemplateBuilder;






class nsContentTestNode : public TestNode
{
public:
    nsContentTestNode(nsXULTemplateQueryProcessorRDF* aProcessor,
                      nsIAtom* aContentVariable);

    virtual nsresult FilterInstantiations(InstantiationSet& aInstantiations,
                                          bool* aCantHandleYet) const MOZ_OVERRIDE;

    nsresult
    Constrain(InstantiationSet& aInstantiations) MOZ_OVERRIDE;

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


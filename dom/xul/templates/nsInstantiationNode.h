




#ifndef nsInstantiationNode_h__
#define nsInstantiationNode_h__

#include "mozilla/Attributes.h"
#include "nsRuleNetwork.h"
#include "nsRDFQuery.h"

class nsXULTemplateQueryProcessorRDF;





class nsInstantiationNode : public ReteNode
{
public:
    nsInstantiationNode(nsXULTemplateQueryProcessorRDF* aProcessor,
                        nsRDFQuery* aRule);

    ~nsInstantiationNode();

    
    virtual nsresult Propagate(InstantiationSet& aInstantiations,
                               bool aIsUpdate, bool& aMatched) MOZ_OVERRIDE;

protected:

    nsXULTemplateQueryProcessorRDF* mProcessor;
    nsRDFQuery* mQuery;
};

#endif 

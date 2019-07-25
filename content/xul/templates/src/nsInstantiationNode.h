






































#ifndef nsInstantiationNode_h__
#define nsInstantiationNode_h__

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
                               PRBool aIsUpdate, PRBool& aMatched);

protected:

    nsXULTemplateQueryProcessorRDF* mProcessor;
    nsRDFQuery* mQuery;
};

#endif 

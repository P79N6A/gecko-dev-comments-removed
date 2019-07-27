




#ifndef nsRDFTestNode_h__
#define nsRDFTestNode_h__

#include "nsRuleNetwork.h"

class nsIRDFResource;
class nsIRDFNode;






class nsRDFTestNode : public TestNode
{
public:
    explicit nsRDFTestNode(TestNode* aParent)
        : TestNode(aParent) {}

    










    virtual bool CanPropagate(nsIRDFResource* aSource,
                                nsIRDFResource* aProperty,
                                nsIRDFNode* aTarget,
                                Instantiation& aInitialBindings) const = 0;

    


    virtual void Retract(nsIRDFResource* aSource,
                         nsIRDFResource* aProperty,
                         nsIRDFNode* aTarget) const = 0;
};

#endif 

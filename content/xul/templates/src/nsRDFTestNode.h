





































#ifndef nsRDFTestNode_h__
#define nsRDFTestNode_h__

#include "nsRuleNetwork.h"

class nsIRDFResource;
class nsIRDFNode;






class nsRDFTestNode : public TestNode
{
public:
    nsRDFTestNode(TestNode* aParent)
        : TestNode(aParent) {}

    










    virtual PRBool CanPropagate(nsIRDFResource* aSource,
                                nsIRDFResource* aProperty,
                                nsIRDFNode* aTarget,
                                Instantiation& aInitialBindings) const = 0;

    


    virtual void Retract(nsIRDFResource* aSource,
                         nsIRDFResource* aProperty,
                         nsIRDFNode* aTarget) const = 0;
};

#endif 

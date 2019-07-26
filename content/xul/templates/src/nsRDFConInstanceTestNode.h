




#ifndef nsRDFConInstanceTestNode_h__
#define nsRDFConInstanceTestNode_h__

#include "nscore.h"
#include "nsRDFTestNode.h"
#include "nsIRDFResource.h"
#include "nsIRDFDataSource.h"
#include "nsXULTemplateQueryProcessorRDF.h"





class nsRDFConInstanceTestNode : public nsRDFTestNode
{
public:
    enum Test { eFalse, eTrue, eDontCare };

    nsRDFConInstanceTestNode(TestNode* aParent,
                             nsXULTemplateQueryProcessorRDF* aProcessor,
                             nsIAtom* aContainerVariable,
                             Test aContainer,
                             Test aEmpty);

    virtual nsresult FilterInstantiations(InstantiationSet& aInstantiations,
                                          bool* aCantHandleYet) const;

    virtual bool
    CanPropagate(nsIRDFResource* aSource,
                 nsIRDFResource* aProperty,
                 nsIRDFNode* aTarget,
                 Instantiation& aInitialBindings) const;

    virtual void
    Retract(nsIRDFResource* aSource,
            nsIRDFResource* aProperty,
            nsIRDFNode* aTarget) const;


    class Element : public MemoryElement {
    public:
        Element(nsIRDFResource* aContainer,
                Test aContainerTest,
                Test aEmptyTest)
            : mContainer(aContainer),
              mContainerTest(aContainerTest),
              mEmptyTest(aEmptyTest) {
            MOZ_COUNT_CTOR(nsRDFConInstanceTestNode::Element); }

        virtual ~Element() { MOZ_COUNT_DTOR(nsRDFConInstanceTestNode::Element); }

        virtual const char* Type() const {
            return "nsRDFConInstanceTestNode::Element"; }

        virtual PLHashNumber Hash() const {
            return mozilla::HashGeneric(mContainerTest, mEmptyTest, mContainer.get());
        }

        virtual bool Equals(const MemoryElement& aElement) const {
            if (aElement.Type() == Type()) {
                const Element& element = static_cast<const Element&>(aElement);
                return mContainer == element.mContainer
                    && mContainerTest == element.mContainerTest
                    && mEmptyTest == element.mEmptyTest;
            }
            return false; }

    protected:
        nsCOMPtr<nsIRDFResource> mContainer;
        Test mContainerTest;
        Test mEmptyTest;
    };

protected:
    nsXULTemplateQueryProcessorRDF* mProcessor;
    nsCOMPtr<nsIAtom> mContainerVariable;
    Test mContainer;
    Test mEmpty;
};

#endif







#ifndef nsRDFPropertyTestNode_h__
#define nsRDFPropertyTestNode_h__

#include "mozilla/Attributes.h"
#include "nscore.h"
#include "nsRDFTestNode.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFResource.h"
#include "nsXULTemplateQueryProcessorRDF.h"

class nsRDFPropertyTestNode : public nsRDFTestNode
{
public:
    


    nsRDFPropertyTestNode(TestNode* aParent,
                          nsXULTemplateQueryProcessorRDF* aProcessor,
                          nsIAtom* aSourceVariable,
                          nsIRDFResource* aProperty,
                          nsIAtom* aTargetVariable);

    


    nsRDFPropertyTestNode(TestNode* aParent,
                          nsXULTemplateQueryProcessorRDF* aProcessor,
                          nsIRDFResource* aSource,
                          nsIRDFResource* aProperty,
                          nsIAtom* aTargetVariable);

    


    nsRDFPropertyTestNode(TestNode* aParent,
                          nsXULTemplateQueryProcessorRDF* aProcessor,
                          nsIAtom* aSourceVariable,
                          nsIRDFResource* aProperty,
                          nsIRDFNode* aTarget);

    virtual nsresult FilterInstantiations(InstantiationSet& aInstantiations,
                                          bool* aCantHandleYet) const override;

    virtual bool
    CanPropagate(nsIRDFResource* aSource,
                 nsIRDFResource* aProperty,
                 nsIRDFNode* aTarget,
                 Instantiation& aInitialBindings) const override;

    virtual void
    Retract(nsIRDFResource* aSource,
            nsIRDFResource* aProperty,
            nsIRDFNode* aTarget) const override;


    class Element : public MemoryElement {
    public:
        Element(nsIRDFResource* aSource,
                nsIRDFResource* aProperty,
                nsIRDFNode* aTarget)
            : mSource(aSource),
              mProperty(aProperty),
              mTarget(aTarget) {
            MOZ_COUNT_CTOR(nsRDFPropertyTestNode::Element); }

        virtual ~Element() { MOZ_COUNT_DTOR(nsRDFPropertyTestNode::Element); }

        virtual const char* Type() const override {
            return "nsRDFPropertyTestNode::Element"; }

        virtual PLHashNumber Hash() const override {
            return mozilla::HashGeneric(mSource.get(), mProperty.get(), mTarget.get());
        }

        virtual bool Equals(const MemoryElement& aElement) const override {
            if (aElement.Type() == Type()) {
                const Element& element = static_cast<const Element&>(aElement);
                return mSource == element.mSource
                    && mProperty == element.mProperty
                    && mTarget == element.mTarget;
            }
            return false; }

    protected:
        nsCOMPtr<nsIRDFResource> mSource;
        nsCOMPtr<nsIRDFResource> mProperty;
        nsCOMPtr<nsIRDFNode> mTarget;
    };

protected:
    nsXULTemplateQueryProcessorRDF* mProcessor;
    nsCOMPtr<nsIAtom>        mSourceVariable;
    nsCOMPtr<nsIRDFResource> mSource;
    nsCOMPtr<nsIRDFResource> mProperty;
    nsCOMPtr<nsIAtom>        mTargetVariable;
    nsCOMPtr<nsIRDFNode>     mTarget;
};

#endif 

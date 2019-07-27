




#include "nsIComponentManager.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIServiceManager.h"
#include "nsRDFCID.h"
#include "nsRDFConInstanceTestNode.h"
#include "nsResourceSet.h"

#include "mozilla/Logging.h"
#include "nsXULContentUtils.h"

using mozilla::LogLevel;

extern PRLogModuleInfo* gXULTemplateLog;

static const char*
TestToString(nsRDFConInstanceTestNode::Test aTest) {
    switch (aTest) {
    case nsRDFConInstanceTestNode::eFalse:    return "false";
    case nsRDFConInstanceTestNode::eTrue:     return "true";
    case nsRDFConInstanceTestNode::eDontCare: return "dontcare";
    }
    return "?";
}

nsRDFConInstanceTestNode::nsRDFConInstanceTestNode(TestNode* aParent,
                                                   nsXULTemplateQueryProcessorRDF* aProcessor,
                                                   nsIAtom* aContainerVariable,
                                                   Test aContainer,
                                                   Test aEmpty)
    : nsRDFTestNode(aParent),
      mProcessor(aProcessor),
      mContainerVariable(aContainerVariable),
      mContainer(aContainer),
      mEmpty(aEmpty)
{
    if (MOZ_LOG_TEST(gXULTemplateLog, LogLevel::Debug)) {
        nsAutoCString props;

        nsResourceSet& containmentProps = aProcessor->ContainmentProperties();
        nsResourceSet::ConstIterator last = containmentProps.Last();
        nsResourceSet::ConstIterator first = containmentProps.First();
        nsResourceSet::ConstIterator iter;

        for (iter = first; iter != last; ++iter) {
            if (iter != first)
                props += " ";

            const char* str;
            iter->GetValueConst(&str);

            props += str;
        }

        nsAutoString cvar(NS_LITERAL_STRING("(none)"));
        if (mContainerVariable)
            mContainerVariable->ToString(cvar);

        MOZ_LOG(gXULTemplateLog, LogLevel::Debug,
               ("nsRDFConInstanceTestNode[%p]: parent=%p member-props=(%s) container-var=%s container=%s empty=%s",
                this,
                aParent,
                props.get(),
                NS_ConvertUTF16toUTF8(cvar).get(),
                TestToString(aContainer),
                TestToString(aEmpty)));
    }
}

nsresult
nsRDFConInstanceTestNode::FilterInstantiations(InstantiationSet& aInstantiations,
                                               bool* aCantHandleYet) const
{
    nsresult rv;

    if (aCantHandleYet)
        *aCantHandleYet = false;

    nsCOMPtr<nsIRDFContainerUtils> rdfc
        = do_GetService("@mozilla.org/rdf/container-utils;1");

    if (! rdfc)
        return NS_ERROR_FAILURE;

    nsIRDFDataSource* ds = mProcessor->GetDataSource();

    InstantiationSet::Iterator last = aInstantiations.Last();
    for (InstantiationSet::Iterator inst = aInstantiations.First(); inst != last; ++inst) {
        nsCOMPtr<nsIRDFNode> value;
        if (! inst->mAssignments.GetAssignmentFor(mContainerVariable, getter_AddRefs(value))) {
            NS_ERROR("can't do unbounded container testing");
            return NS_ERROR_UNEXPECTED;
        }

        nsCOMPtr<nsIRDFResource> valueres = do_QueryInterface(value);
        if (! valueres) {
            aInstantiations.Erase(inst--);
            continue;
        }

        if (MOZ_LOG_TEST(gXULTemplateLog, LogLevel::Debug)) {
            const char* container = "(unbound)";
            valueres->GetValueConst(&container);

            MOZ_LOG(gXULTemplateLog, LogLevel::Debug,
                   ("nsRDFConInstanceTestNode[%p]::FilterInstantiations() container=[%s]",
                    this, container));
        }

        nsCOMPtr<nsIRDFContainer> rdfcontainer;

        bool isRDFContainer;
        rv = rdfc->IsContainer(ds, valueres, &isRDFContainer);
        if (NS_FAILED(rv)) return rv;

        if (mEmpty != eDontCare || mContainer != eDontCare) {
            Test empty = eDontCare;
            Test container = eDontCare;

            if (isRDFContainer) {
                
                
                container = eTrue;

                
                rdfcontainer = do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
                if (NS_FAILED(rv)) return rv;

                rv = rdfcontainer->Init(ds, valueres);
                if (NS_FAILED(rv)) return rv;

                int32_t count;
                rv = rdfcontainer->GetCount(&count);
                if (NS_FAILED(rv)) return rv;

                empty = (count == 0) ? eTrue : eFalse;
            } else {
                empty = eTrue;
                container = eFalse;

                
                
                
                nsResourceSet& containmentProps = mProcessor->ContainmentProperties();
                for (nsResourceSet::ConstIterator property = containmentProps.First();
                     property != containmentProps.Last();
                     ++property) {
                    nsCOMPtr<nsIRDFNode> target;
                    rv = ds->GetTarget(valueres, *property, true, getter_AddRefs(target));
                    if (NS_FAILED(rv)) return rv;

                    if (target != nullptr) {
                        
                        empty = eFalse;
                        container = eTrue;
                        break;
                    }
                }

                
                
                
                if (container == eFalse && mContainer != eDontCare) {
                    nsCOMPtr<nsISimpleEnumerator> arcsout;
                    rv = ds->ArcLabelsOut(valueres, getter_AddRefs(arcsout));
                    if (NS_FAILED(rv)) return rv;

                    while (1) {
                        bool hasmore;
                        rv = arcsout->HasMoreElements(&hasmore);
                        if (NS_FAILED(rv)) return rv;

                        if (! hasmore)
                            break;

                        nsCOMPtr<nsISupports> isupports;
                        rv = arcsout->GetNext(getter_AddRefs(isupports));
                        if (NS_FAILED(rv)) return rv;

                        nsCOMPtr<nsIRDFResource> property = do_QueryInterface(isupports);
                        NS_ASSERTION(property != nullptr, "not a property");
                        if (! property)
                            return NS_ERROR_UNEXPECTED;

                        if (mProcessor->ContainmentProperties().Contains(property)) {
                            container = eTrue;
                            break;
                        }
                    }
                }
            }

            MOZ_LOG(gXULTemplateLog, LogLevel::Debug,
                   ("    empty => %s",
                    (empty == mEmpty) ? "consistent" : "inconsistent"));

            MOZ_LOG(gXULTemplateLog, LogLevel::Debug,
                   ("    container => %s",
                    (container == mContainer) ? "consistent" : "inconsistent"));

            if (((mEmpty == empty) && (mContainer == container)) ||
                ((mEmpty == eDontCare) && (mContainer == container)) ||
                ((mContainer == eDontCare) && (mEmpty == empty)))
            {
                Element* element =
                    new nsRDFConInstanceTestNode::Element(valueres, container, empty);

                if (! element)
                    return NS_ERROR_OUT_OF_MEMORY;

                inst->AddSupportingElement(element);
            }
            else {
                aInstantiations.Erase(inst--);
            }
        }
    }

    return NS_OK;
}

bool
nsRDFConInstanceTestNode::CanPropagate(nsIRDFResource* aSource,
                                       nsIRDFResource* aProperty,
                                       nsIRDFNode* aTarget,
                                       Instantiation& aInitialBindings) const
{
    nsresult rv;

    bool canpropagate = false;

    nsCOMPtr<nsIRDFContainerUtils> rdfc
        = do_GetService("@mozilla.org/rdf/container-utils;1");

    if (! rdfc)
        return false;

    
    rv = rdfc->IsOrdinalProperty(aProperty, &canpropagate);
    if (NS_FAILED(rv)) return false;

    if (! canpropagate) {
        canpropagate = mProcessor->ContainmentProperties().Contains(aProperty);
    }

    if (MOZ_LOG_TEST(gXULTemplateLog, LogLevel::Debug)) {
        const char* source;
        aSource->GetValueConst(&source);

        const char* property;
        aProperty->GetValueConst(&property);

        nsAutoString target;
        nsXULContentUtils::GetTextForNode(aTarget, target);

        MOZ_LOG(gXULTemplateLog, LogLevel::Debug,
               ("nsRDFConInstanceTestNode[%p]: CanPropagate([%s]==[%s]=>[%s]) => %s",
                this, source, property, NS_ConvertUTF16toUTF8(target).get(),
                canpropagate ? "true" : "false"));
    }

    if (canpropagate) {
        aInitialBindings.AddAssignment(mContainerVariable, aSource);
        return true;
    }

    return false;
}

void
nsRDFConInstanceTestNode::Retract(nsIRDFResource* aSource,
                                  nsIRDFResource* aProperty,
                                  nsIRDFNode* aTarget) const
{
    
    if (0) {
        mProcessor->RetractElement(Element(aSource, mContainer, mEmpty));
    }
}


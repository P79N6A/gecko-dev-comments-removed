




#include "nsRDFConMemberTestNode.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsRDFCID.h"
#include "nsIServiceManager.h"
#include "nsResourceSet.h"
#include "nsString.h"
#include "nsXULContentUtils.h"

#include "prlog.h"
extern PRLogModuleInfo* gXULTemplateLog;

nsRDFConMemberTestNode::nsRDFConMemberTestNode(TestNode* aParent,
                                               nsXULTemplateQueryProcessorRDF* aProcessor,
                                               nsIAtom *aContainerVariable,
                                               nsIAtom *aMemberVariable)
    : nsRDFTestNode(aParent),
      mProcessor(aProcessor),
      mContainerVariable(aContainerVariable),
      mMemberVariable(aMemberVariable)
{
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
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

        nsAutoString mvar(NS_LITERAL_STRING("(none)"));
        if (mMemberVariable)
            mMemberVariable->ToString(mvar);

        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("nsRDFConMemberTestNode[%p]: parent=%p member-props=(%s) container-var=%s member-var=%s",
                this,
                aParent,
                props.get(),
                NS_ConvertUTF16toUTF8(cvar).get(),
                NS_ConvertUTF16toUTF8(mvar).get()));
    }
}

nsresult
nsRDFConMemberTestNode::FilterInstantiations(InstantiationSet& aInstantiations,
                                             bool* aCantHandleYet) const
{
    
    nsresult rv;

    if (aCantHandleYet)
        *aCantHandleYet = false;

    nsCOMPtr<nsIRDFContainerUtils> rdfc =
        do_GetService("@mozilla.org/rdf/container-utils;1");

    if (! rdfc)
        return NS_ERROR_FAILURE;

    nsIRDFDataSource* ds = mProcessor->GetDataSource();

    InstantiationSet::Iterator last = aInstantiations.Last();
    for (InstantiationSet::Iterator inst = aInstantiations.First(); inst != last; ++inst) {
        bool hasContainerBinding;
        nsCOMPtr<nsIRDFNode> containerValue;
        hasContainerBinding = inst->mAssignments.GetAssignmentFor(mContainerVariable,
                                                                  getter_AddRefs(containerValue));

        nsCOMPtr<nsIRDFResource> containerRes = do_QueryInterface(containerValue);

        nsCOMPtr<nsIRDFContainer> rdfcontainer;

        if (hasContainerBinding && containerRes) {
            
            
            
            bool isRDFContainer;
            rv = rdfc->IsContainer(ds, containerRes, &isRDFContainer);
            if (NS_FAILED(rv)) return rv;

            if (isRDFContainer) {
                rdfcontainer = do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
                if (NS_FAILED(rv)) return rv;

                rv = rdfcontainer->Init(ds, containerRes);
                if (NS_FAILED(rv)) return rv;
            }
        }

        bool hasMemberBinding;
        nsCOMPtr<nsIRDFNode> memberValue;
        hasMemberBinding = inst->mAssignments.GetAssignmentFor(mMemberVariable,
                                                               getter_AddRefs(memberValue));

        if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
            const char* container = "(unbound)";
            if (hasContainerBinding)
                containerRes->GetValueConst(&container);

            nsAutoString member(NS_LITERAL_STRING("(unbound)"));
            if (hasMemberBinding)
                nsXULContentUtils::GetTextForNode(memberValue, member);

            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("nsRDFConMemberTestNode[%p]: FilterInstantiations() container=[%s] member=[%s]",
                    this, container, NS_ConvertUTF16toUTF8(member).get()));
        }

        if (hasContainerBinding && hasMemberBinding) {
            
            bool isconsistent = false;

            if (rdfcontainer) {
                
                int32_t index;
                rv = rdfcontainer->IndexOf(memberValue, &index);
                if (NS_FAILED(rv)) return rv;

                if (index >= 0)
                    isconsistent = true;
            }

            
            
            

            if (! isconsistent) {
                
                
                
                nsResourceSet& containmentProps = mProcessor->ContainmentProperties();
                for (nsResourceSet::ConstIterator property = containmentProps.First();
                     property != containmentProps.Last();
                     ++property) {
                    bool hasAssertion;
                    rv = ds->HasAssertion(containerRes,
                                          *property,
                                          memberValue,
                                          true,
                                          &hasAssertion);
                    if (NS_FAILED(rv)) return rv;

                    if (hasAssertion) {
                        
                        
                        isconsistent = true;
                        break;
                    }
                }
            }

            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("    consistency check => %s", isconsistent ? "passed" : "failed"));

            if (isconsistent) {
                
                Element* element =
                    new nsRDFConMemberTestNode::Element(containerRes,
                                                        memberValue);

                if (! element)
                    return NS_ERROR_OUT_OF_MEMORY;

                inst->AddSupportingElement(element);
            }
            else {
                
                aInstantiations.Erase(inst--);
            }

            
            continue;
        }

        if (hasContainerBinding && rdfcontainer) {
            
            
            
            nsCOMPtr<nsISimpleEnumerator> elements;
            rv = rdfcontainer->GetElements(getter_AddRefs(elements));
            if (NS_FAILED(rv)) return rv;

            while (1) {
                bool hasmore;
                rv = elements->HasMoreElements(&hasmore);
                if (NS_FAILED(rv)) return rv;

                if (! hasmore)
                    break;

                nsCOMPtr<nsISupports> isupports;
                rv = elements->GetNext(getter_AddRefs(isupports));
                if (NS_FAILED(rv)) return rv;

                nsCOMPtr<nsIRDFNode> node = do_QueryInterface(isupports);
                if (! node)
                    return NS_ERROR_UNEXPECTED;

                if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                    nsAutoString member;
                    nsXULContentUtils::GetTextForNode(node, member);

                    PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                           ("    member => %s", NS_ConvertUTF16toUTF8(member).get()));
                }

                Instantiation newinst = *inst;
                newinst.AddAssignment(mMemberVariable, node);

                Element* element =
                    new nsRDFConMemberTestNode::Element(containerRes, node);

                if (! element)
                    return NS_ERROR_OUT_OF_MEMORY;

                newinst.AddSupportingElement(element);
                aInstantiations.Insert(inst, newinst);
            }
        }

        if (hasMemberBinding) {
            
            
            
            
            
            nsCOMPtr<nsISimpleEnumerator> arcsin;
            rv = ds->ArcLabelsIn(memberValue, getter_AddRefs(arcsin));
            if (NS_FAILED(rv)) return rv;

            while (1) {
                nsCOMPtr<nsIRDFResource> property;

                {
                    bool hasmore;
                    rv = arcsin->HasMoreElements(&hasmore);
                    if (NS_FAILED(rv)) return rv;

                    if (! hasmore)
                        break;

                    nsCOMPtr<nsISupports> isupports;
                    rv = arcsin->GetNext(getter_AddRefs(isupports));
                    if (NS_FAILED(rv)) return rv;

                    property = do_QueryInterface(isupports);
                    if (! property)
                        return NS_ERROR_UNEXPECTED;
                }

                
                
                
                
                
                bool isordinal;
                rv = rdfc->IsOrdinalProperty(property, &isordinal);
                if (NS_FAILED(rv)) return rv;

                if (isordinal) {
                    
                    
                    
                    
                    nsCOMPtr<nsISimpleEnumerator> sources;
                    rv = ds->GetSources(property, memberValue, true,
                                        getter_AddRefs(sources));
                    if (NS_FAILED(rv)) return rv;

                    while (1) {
                        bool hasmore;
                        rv = sources->HasMoreElements(&hasmore);
                        if (NS_FAILED(rv)) return rv;

                        if (! hasmore)
                            break;

                        nsCOMPtr<nsISupports> isupports;
                        rv = sources->GetNext(getter_AddRefs(isupports));
                        if (NS_FAILED(rv)) return rv;

                        nsCOMPtr<nsIRDFResource> source = do_QueryInterface(isupports);
                        if (! source)
                            return NS_ERROR_UNEXPECTED;

                        if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                            const char* container;
                            source->GetValueConst(&container);

                            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                                   ("    container => %s", container));
                        }

                        
                        Instantiation newinst = *inst;
                        newinst.AddAssignment(mContainerVariable, source);

                        Element* element =
                            new nsRDFConMemberTestNode::Element(source,
                                                                memberValue);

                        if (! element)
                            return NS_ERROR_OUT_OF_MEMORY;

                        newinst.AddSupportingElement(element);

                        aInstantiations.Insert(inst, newinst);
                    }
                }
            }
        }

        if ((hasContainerBinding && ! hasMemberBinding) ||
            (! hasContainerBinding && hasMemberBinding)) {
            
            
            
            nsResourceSet& containmentProps = mProcessor->ContainmentProperties();
            for (nsResourceSet::ConstIterator property = containmentProps.First();
                 property != containmentProps.Last();
                 ++property) {
                nsCOMPtr<nsISimpleEnumerator> results;
                if (hasContainerBinding) {
                    rv = ds->GetTargets(containerRes, *property, true,
                                        getter_AddRefs(results));
                }
                else {
                    rv = ds->GetSources(*property, memberValue, true,
                                        getter_AddRefs(results));
                }
                if (NS_FAILED(rv)) return rv;

                while (1) {
                    bool hasmore;
                    rv = results->HasMoreElements(&hasmore);
                    if (NS_FAILED(rv)) return rv;

                    if (! hasmore)
                        break;

                    nsCOMPtr<nsISupports> isupports;
                    rv = results->GetNext(getter_AddRefs(isupports));
                    if (NS_FAILED(rv)) return rv;

                    nsIAtom* variable;
                    nsCOMPtr<nsIRDFNode> value;
                    nsCOMPtr<nsIRDFResource> valueRes;

                    if (hasContainerBinding) {
                        variable = mMemberVariable;

                        value = do_QueryInterface(isupports);
                        NS_ASSERTION(value != nullptr, "member is not an nsIRDFNode");
                        if (! value) continue;

                        if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                            nsAutoString s;
                            nsXULContentUtils::GetTextForNode(value, s);

                            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                                   ("    member => %s", NS_ConvertUTF16toUTF8(s).get()));
                        }
                    }
                    else {
                        variable = mContainerVariable;

                        valueRes = do_QueryInterface(isupports);
                        NS_ASSERTION(valueRes != nullptr, "container is not an nsIRDFResource");
                        if (! valueRes) continue;

                        value = valueRes;

                        if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                            const char* s;
                            valueRes->GetValueConst(&s);

                            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                                   ("    container => %s", s));
                        }
                    }

                    
                    
                    
                    Instantiation newinst = *inst;
                    newinst.AddAssignment(variable, value);

                    Element* element;
                    if (hasContainerBinding) {
                        element =
                            new nsRDFConMemberTestNode::Element(containerRes, value);
                    }
                    else {
                        element =
                            new nsRDFConMemberTestNode::Element(valueRes, memberValue);
                    }

                    if (! element)
                        return NS_ERROR_OUT_OF_MEMORY;

                    newinst.AddSupportingElement(element);

                    aInstantiations.Insert(inst, newinst);
                }
            }
        }

        if (! hasContainerBinding && ! hasMemberBinding) {
            
            if (!aCantHandleYet) {
                nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_MEMBER_UNBOUND);
                return NS_ERROR_UNEXPECTED;
            }

            *aCantHandleYet = true;
            return NS_OK;
        }

        
        aInstantiations.Erase(inst--);
    }

    return NS_OK;
}

bool
nsRDFConMemberTestNode::CanPropagate(nsIRDFResource* aSource,
                                     nsIRDFResource* aProperty,
                                     nsIRDFNode* aTarget,
                                     Instantiation& aInitialBindings) const
{
    nsresult rv;

    bool canpropagate = false;

    nsCOMPtr<nsIRDFContainerUtils> rdfc =
        do_GetService("@mozilla.org/rdf/container-utils;1");

    if (! rdfc)
        return false;

    
    rv = rdfc->IsOrdinalProperty(aProperty, &canpropagate);
    if (NS_FAILED(rv)) return false;

    if (! canpropagate) {
        canpropagate = mProcessor->ContainmentProperties().Contains(aProperty);
    }

    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        const char* source;
        aSource->GetValueConst(&source);

        const char* property;
        aProperty->GetValueConst(&property);

        nsAutoString target;
        nsXULContentUtils::GetTextForNode(aTarget, target);

        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("nsRDFConMemberTestNode[%p]: CanPropagate([%s]==[%s]=>[%s]) => %s",
                this, source, property, NS_ConvertUTF16toUTF8(target).get(),
                canpropagate ? "true" : "false"));
    }

    if (canpropagate) {
        aInitialBindings.AddAssignment(mContainerVariable, aSource);
        aInitialBindings.AddAssignment(mMemberVariable, aTarget);
        return true;
    }

    return false;
}

void
nsRDFConMemberTestNode::Retract(nsIRDFResource* aSource,
                                nsIRDFResource* aProperty,
                                nsIRDFNode* aTarget) const
{
    bool canretract = false;

    nsCOMPtr<nsIRDFContainerUtils> rdfc =
        do_GetService("@mozilla.org/rdf/container-utils;1");

    if (! rdfc)
        return;

    
    nsresult rv;
    rv = rdfc->IsOrdinalProperty(aProperty, &canretract);
    if (NS_FAILED(rv)) return;

    if (! canretract) {
        canretract = mProcessor->ContainmentProperties().Contains(aProperty);
    }

    if (canretract) {
        mProcessor->RetractElement(Element(aSource, aTarget));
    }
}

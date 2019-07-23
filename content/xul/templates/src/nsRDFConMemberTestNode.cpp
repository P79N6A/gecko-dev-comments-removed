





































#include "nsRDFConMemberTestNode.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsRDFCID.h"
#include "nsIServiceManager.h"
#include "nsResourceSet.h"
#include "nsString.h"

#include "prlog.h"
#ifdef PR_LOGGING
#include "nsXULContentUtils.h"
extern PRLogModuleInfo* gXULTemplateLog;
#endif

nsRDFConMemberTestNode::nsRDFConMemberTestNode(TestNode* aParent,
                                               nsXULTemplateQueryProcessorRDF* aProcessor,
                                               nsIAtom *aContainerVariable,
                                               nsIAtom *aMemberVariable)
    : nsRDFTestNode(aParent),
      mProcessor(aProcessor),
      mContainerVariable(aContainerVariable),
      mMemberVariable(aMemberVariable)
{
#ifdef PR_LOGGING
    if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        nsCAutoString props;

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
#endif
}

nsresult
nsRDFConMemberTestNode::FilterInstantiations(InstantiationSet& aInstantiations,
                                             PRBool* aCantHandleYet) const
{
    
    nsresult rv;

    if (aCantHandleYet)
        *aCantHandleYet = PR_FALSE;

    nsCOMPtr<nsIRDFContainerUtils> rdfc =
        do_GetService("@mozilla.org/rdf/container-utils;1");

    if (! rdfc)
        return NS_ERROR_FAILURE;

    nsIRDFDataSource* ds = mProcessor->GetDataSource();

    InstantiationSet::Iterator last = aInstantiations.Last();
    for (InstantiationSet::Iterator inst = aInstantiations.First(); inst != last; ++inst) {
        PRBool hasContainerBinding;
        nsCOMPtr<nsIRDFNode> containerValue;
        hasContainerBinding = inst->mAssignments.GetAssignmentFor(mContainerVariable,
                                                                  getter_AddRefs(containerValue));

        nsCOMPtr<nsIRDFResource> containerRes = do_QueryInterface(containerValue);

        nsCOMPtr<nsIRDFContainer> rdfcontainer;

        if (hasContainerBinding && containerRes) {
            
            
            
            PRBool isRDFContainer;
            rv = rdfc->IsContainer(ds, containerRes, &isRDFContainer);
            if (NS_FAILED(rv)) return rv;

            if (isRDFContainer) {
                rdfcontainer = do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
                if (NS_FAILED(rv)) return rv;

                rv = rdfcontainer->Init(ds, containerRes);
                if (NS_FAILED(rv)) return rv;
            }
        }

        PRBool hasMemberBinding;
        nsCOMPtr<nsIRDFNode> memberValue;
        hasMemberBinding = inst->mAssignments.GetAssignmentFor(mMemberVariable,
                                                               getter_AddRefs(memberValue));

#ifdef PR_LOGGING
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
#endif

        if (hasContainerBinding && hasMemberBinding) {
            
            PRBool isconsistent = PR_FALSE;

            if (rdfcontainer) {
                
                PRInt32 index;
                rv = rdfcontainer->IndexOf(memberValue, &index);
                if (NS_FAILED(rv)) return rv;

                if (index >= 0)
                    isconsistent = PR_TRUE;
            }

            
            
            

            if (! isconsistent) {
                
                
                
                nsResourceSet& containmentProps = mProcessor->ContainmentProperties();
                for (nsResourceSet::ConstIterator property = containmentProps.First();
                     property != containmentProps.Last();
                     ++property) {
                    PRBool hasAssertion;
                    rv = ds->HasAssertion(containerRes,
                                          *property,
                                          memberValue,
                                          PR_TRUE,
                                          &hasAssertion);
                    if (NS_FAILED(rv)) return rv;

                    if (hasAssertion) {
                        
                        
                        isconsistent = PR_TRUE;
                        break;
                    }
                }
            }

            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("    consistency check => %s", isconsistent ? "passed" : "failed"));

            if (isconsistent) {
                
                Element* element =
                    nsRDFConMemberTestNode::Element::Create(containerRes,
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
                PRBool hasmore;
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

#ifdef PR_LOGGING
                if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                    nsAutoString member;
                    nsXULContentUtils::GetTextForNode(node, member);

                    PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                           ("    member => %s", NS_ConvertUTF16toUTF8(member).get()));
                }
#endif

                Instantiation newinst = *inst;
                newinst.AddAssignment(mMemberVariable, node);

                Element* element =
                    nsRDFConMemberTestNode::Element::Create(containerRes, node);

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
                    PRBool hasmore;
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

                
                
                
                
                
                PRBool isordinal;
                rv = rdfc->IsOrdinalProperty(property, &isordinal);
                if (NS_FAILED(rv)) return rv;

                if (isordinal) {
                    
                    
                    
                    
                    nsCOMPtr<nsISimpleEnumerator> sources;
                    rv = ds->GetSources(property, memberValue, PR_TRUE,
                                        getter_AddRefs(sources));
                    if (NS_FAILED(rv)) return rv;

                    while (1) {
                        PRBool hasmore;
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

#ifdef PR_LOGGING
                        if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                            const char* container;
                            source->GetValueConst(&container);

                            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                                   ("    container => %s", container));
                        }
#endif

                        
                        Instantiation newinst = *inst;
                        newinst.AddAssignment(mContainerVariable, source);

                        Element* element =
                            nsRDFConMemberTestNode::Element::Create(source,
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
                    rv = ds->GetTargets(containerRes, *property, PR_TRUE,
                                        getter_AddRefs(results));
                }
                else {
                    rv = ds->GetSources(*property, memberValue, PR_TRUE,
                                        getter_AddRefs(results));
                }
                if (NS_FAILED(rv)) return rv;

                while (1) {
                    PRBool hasmore;
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
                        NS_ASSERTION(value != nsnull, "member is not an nsIRDFNode");
                        if (! value) continue;

#ifdef PR_LOGGING
                        if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                            nsAutoString s;
                            nsXULContentUtils::GetTextForNode(value, s);

                            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                                   ("    member => %s", NS_ConvertUTF16toUTF8(s).get()));
                        }
#endif
                    }
                    else {
                        variable = mContainerVariable;

                        valueRes = do_QueryInterface(isupports);
                        NS_ASSERTION(valueRes != nsnull, "container is not an nsIRDFResource");
                        if (! valueRes) continue;

                        value = valueRes;

#ifdef PR_LOGGING
                        if (PR_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                            const char* s;
                            valueRes->GetValueConst(&s);

                            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                                   ("    container => %s", s));
                        }
#endif
                    }

                    
                    
                    
                    Instantiation newinst = *inst;
                    newinst.AddAssignment(variable, value);

                    Element* element;
                    if (hasContainerBinding) {
                        element =
                            nsRDFConMemberTestNode::Element::Create(containerRes, value);
                    }
                    else {
                        element =
                            nsRDFConMemberTestNode::Element::Create(valueRes, memberValue);
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
                return NS_ERROR_UNEXPECTED;
            }

            *aCantHandleYet = PR_TRUE;
            return NS_OK;
        }

        
        aInstantiations.Erase(inst--);
    }

    return NS_OK;
}

PRBool
nsRDFConMemberTestNode::CanPropagate(nsIRDFResource* aSource,
                                     nsIRDFResource* aProperty,
                                     nsIRDFNode* aTarget,
                                     Instantiation& aInitialBindings) const
{
    nsresult rv;

    PRBool canpropagate = PR_FALSE;

    nsCOMPtr<nsIRDFContainerUtils> rdfc =
        do_GetService("@mozilla.org/rdf/container-utils;1");

    if (! rdfc)
        return NS_ERROR_FAILURE;

    
    rv = rdfc->IsOrdinalProperty(aProperty, &canpropagate);
    if (NS_FAILED(rv)) return PR_FALSE;

    if (! canpropagate) {
        canpropagate = mProcessor->ContainmentProperties().Contains(aProperty);
    }

#ifdef PR_LOGGING
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
#endif

    if (canpropagate) {
        aInitialBindings.AddAssignment(mContainerVariable, aSource);
        aInitialBindings.AddAssignment(mMemberVariable, aTarget);
        return PR_TRUE;
    }

    return PR_FALSE;
}

void
nsRDFConMemberTestNode::Retract(nsIRDFResource* aSource,
                                nsIRDFResource* aProperty,
                                nsIRDFNode* aTarget) const
{
    PRBool canretract = PR_FALSE;

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

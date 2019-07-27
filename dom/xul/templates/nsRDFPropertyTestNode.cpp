




#include "nsRDFPropertyTestNode.h"
#include "nsString.h"
#include "nsXULContentUtils.h"

#include "mozilla/Logging.h"
extern PRLogModuleInfo* gXULTemplateLog;
#include "nsIRDFLiteral.h"

nsRDFPropertyTestNode::nsRDFPropertyTestNode(TestNode* aParent,
                                             nsXULTemplateQueryProcessorRDF* aProcessor,
                                             nsIAtom* aSourceVariable,
                                             nsIRDFResource* aProperty,
                                             nsIAtom* aTargetVariable)
    : nsRDFTestNode(aParent),
      mProcessor(aProcessor),
      mSourceVariable(aSourceVariable),
      mSource(nullptr),
      mProperty(aProperty),
      mTargetVariable(aTargetVariable),
      mTarget(nullptr)
{
    if (MOZ_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        const char* prop = "(null)";
        if (aProperty)
            aProperty->GetValueConst(&prop);

        nsAutoString svar(NS_LITERAL_STRING("(none)"));
        if (mSourceVariable)
            mSourceVariable->ToString(svar);

        nsAutoString tvar(NS_LITERAL_STRING("(none)"));
        if (mTargetVariable)
            mTargetVariable->ToString(tvar);

        MOZ_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("nsRDFPropertyTestNode[%p]: parent=%p source=%s property=%s target=%s",
                this, aParent, NS_ConvertUTF16toUTF8(svar).get(), prop, NS_ConvertUTF16toUTF8(tvar).get()));
    }
}


nsRDFPropertyTestNode::nsRDFPropertyTestNode(TestNode* aParent,
                                             nsXULTemplateQueryProcessorRDF* aProcessor,
                                             nsIRDFResource* aSource,
                                             nsIRDFResource* aProperty,
                                             nsIAtom* aTargetVariable)
    : nsRDFTestNode(aParent),
      mProcessor(aProcessor),
      mSourceVariable(0),
      mSource(aSource),
      mProperty(aProperty),
      mTargetVariable(aTargetVariable),
      mTarget(nullptr)
{
    if (MOZ_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        const char* source = "(null)";
        if (aSource)
            aSource->GetValueConst(&source);

        const char* prop = "(null)";
        if (aProperty)
            aProperty->GetValueConst(&prop);

        nsAutoString tvar(NS_LITERAL_STRING("(none)"));
        if (mTargetVariable)
            mTargetVariable->ToString(tvar);

        MOZ_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("nsRDFPropertyTestNode[%p]: parent=%p source=%s property=%s target=%s",
                this, aParent, source, prop, NS_ConvertUTF16toUTF8(tvar).get()));
    }
}


nsRDFPropertyTestNode::nsRDFPropertyTestNode(TestNode* aParent,
                                             nsXULTemplateQueryProcessorRDF* aProcessor,
                                             nsIAtom* aSourceVariable,
                                             nsIRDFResource* aProperty,
                                             nsIRDFNode* aTarget)
    : nsRDFTestNode(aParent),
      mProcessor(aProcessor),
      mSourceVariable(aSourceVariable),
      mSource(nullptr),
      mProperty(aProperty),
      mTargetVariable(0),
      mTarget(aTarget)
{
    if (MOZ_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        nsAutoString svar(NS_LITERAL_STRING("(none)"));
        if (mSourceVariable)
            mSourceVariable->ToString(svar);

        const char* prop = "(null)";
        if (aProperty)
            aProperty->GetValueConst(&prop);

        nsAutoString target;
        nsXULContentUtils::GetTextForNode(aTarget, target);

        MOZ_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("nsRDFPropertyTestNode[%p]: parent=%p source=%s property=%s target=%s",
                this, aParent, NS_ConvertUTF16toUTF8(svar).get(), prop, NS_ConvertUTF16toUTF8(target).get()));
    }
}


nsresult
nsRDFPropertyTestNode::FilterInstantiations(InstantiationSet& aInstantiations,
                                            bool* aCantHandleYet) const
{
    nsresult rv;

    if (aCantHandleYet)
        *aCantHandleYet = false;

    nsIRDFDataSource* ds = mProcessor->GetDataSource();

    InstantiationSet::Iterator last = aInstantiations.Last();
    for (InstantiationSet::Iterator inst = aInstantiations.First(); inst != last; ++inst) {
        bool hasSourceBinding;
        nsCOMPtr<nsIRDFResource> sourceRes;

        if (mSource) {
            hasSourceBinding = true;
            sourceRes = mSource;
        }
        else {
            nsCOMPtr<nsIRDFNode> sourceValue;
            hasSourceBinding = inst->mAssignments.GetAssignmentFor(mSourceVariable,
                                                                   getter_AddRefs(sourceValue));
            sourceRes = do_QueryInterface(sourceValue);
        }

        bool hasTargetBinding;
        nsCOMPtr<nsIRDFNode> targetValue;

        if (mTarget) {
            hasTargetBinding = true;
            targetValue = mTarget;
        }
        else {
            hasTargetBinding = inst->mAssignments.GetAssignmentFor(mTargetVariable,
                                                                   getter_AddRefs(targetValue));
        }

        if (MOZ_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
            const char* source = "(unbound)";
            if (hasSourceBinding)
                sourceRes->GetValueConst(&source);

            nsAutoString target(NS_LITERAL_STRING("(unbound)"));
            if (hasTargetBinding)
                nsXULContentUtils::GetTextForNode(targetValue, target);

            MOZ_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("nsRDFPropertyTestNode[%p]: FilterInstantiations() source=[%s] target=[%s]",
                    this, source, NS_ConvertUTF16toUTF8(target).get()));
        }

        if (hasSourceBinding && hasTargetBinding) {
            
            bool hasAssertion;
            rv = ds->HasAssertion(sourceRes, mProperty, targetValue,
                                  true, &hasAssertion);
            if (NS_FAILED(rv)) return rv;

            MOZ_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("    consistency check => %s", hasAssertion ? "passed" : "failed"));

            if (hasAssertion) {
                
                Element* element =
                    new nsRDFPropertyTestNode::Element(sourceRes, mProperty,
                                                       targetValue);

                if (! element)
                    return NS_ERROR_OUT_OF_MEMORY;

                inst->AddSupportingElement(element);
            }
            else {
                
                aInstantiations.Erase(inst--);
            }
        }
        else if ((hasSourceBinding && ! hasTargetBinding) ||
                 (! hasSourceBinding && hasTargetBinding)) {
            
            
            
            nsCOMPtr<nsISimpleEnumerator> results;
            if (hasSourceBinding) {
                rv = ds->GetTargets(sourceRes,
                                    mProperty,
                                    true,
                                    getter_AddRefs(results));
            }
            else {
                rv = ds->GetSources(mProperty,
                                    targetValue,
                                    true,
                                    getter_AddRefs(results));
                if (NS_FAILED(rv)) return rv;
            }

            while (1) {
                bool hasMore;
                rv = results->HasMoreElements(&hasMore);
                if (NS_FAILED(rv)) return rv;

                if (! hasMore)
                    break;

                nsCOMPtr<nsISupports> isupports;
                rv = results->GetNext(getter_AddRefs(isupports));
                if (NS_FAILED(rv)) return rv;

                nsIAtom* variable;
                nsCOMPtr<nsIRDFNode> value;

                if (hasSourceBinding) {
                    variable = mTargetVariable;

                    value = do_QueryInterface(isupports);
                    NS_ASSERTION(value != nullptr, "target is not an nsIRDFNode");

                    if (MOZ_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                        nsAutoString s(NS_LITERAL_STRING("(none found)"));
                        if (value)
                            nsXULContentUtils::GetTextForNode(value, s);

                        MOZ_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                               ("    target => %s", NS_ConvertUTF16toUTF8(s).get()));
                    }

                    if (! value) continue;

                    targetValue = value;
                }
                else {
                    variable = mSourceVariable;

                    nsCOMPtr<nsIRDFResource> source = do_QueryInterface(isupports);
                    NS_ASSERTION(source != nullptr, "source is not an nsIRDFResource");

                    if (MOZ_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
                        const char* s = "(none found)";
                        if (source)
                            source->GetValueConst(&s);

                        MOZ_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                               ("    source => %s", s));
                    }

                    if (! source) continue;

                    value = sourceRes = source;
                }

                
                
                
                Instantiation newinst = *inst;
                newinst.AddAssignment(variable, value);

                Element* element =
                    new nsRDFPropertyTestNode::Element(sourceRes, mProperty,
                                                       targetValue);

                if (! element)
                    return NS_ERROR_OUT_OF_MEMORY;

                newinst.AddSupportingElement(element);

                aInstantiations.Insert(inst, newinst);
            }

            
            aInstantiations.Erase(inst--);
        }
        else {
            if (!aCantHandleYet) {
                nsXULContentUtils::LogTemplateError(ERROR_TEMPLATE_TRIPLE_UNBOUND);
                
                return NS_ERROR_UNEXPECTED;
            }

            *aCantHandleYet = true;
            return NS_OK;
        }
    }

    return NS_OK;
}

bool
nsRDFPropertyTestNode::CanPropagate(nsIRDFResource* aSource,
                                    nsIRDFResource* aProperty,
                                    nsIRDFNode* aTarget,
                                    Instantiation& aInitialBindings) const
{
    bool result;

    if ((mProperty.get() != aProperty) ||
        (mSource && mSource.get() != aSource) ||
        (mTarget && mTarget.get() != aTarget)) {
        result = false;
    }
    else {
        if (mSourceVariable)
            aInitialBindings.AddAssignment(mSourceVariable, aSource);

        if (mTargetVariable)
            aInitialBindings.AddAssignment(mTargetVariable, aTarget);

        result = true;
    }

    if (MOZ_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
        const char* source;
        aSource->GetValueConst(&source);

        const char* property;
        aProperty->GetValueConst(&property);

        nsAutoString target;
        nsXULContentUtils::GetTextForNode(aTarget, target);

        MOZ_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("nsRDFPropertyTestNode[%p]: CanPropagate([%s]==[%s]=>[%s]) => %s",
                this, source, property, NS_ConvertUTF16toUTF8(target).get(),
                result ? "true" : "false"));
    }

    return result;
}

void
nsRDFPropertyTestNode::Retract(nsIRDFResource* aSource,
                               nsIRDFResource* aProperty,
                               nsIRDFNode* aTarget) const
{
    if (aProperty == mProperty.get()) {
        if (MOZ_LOG_TEST(gXULTemplateLog, PR_LOG_DEBUG)) {
            const char* source;
            aSource->GetValueConst(&source);

            const char* property;
            aProperty->GetValueConst(&property);

            nsAutoString target;
            nsXULContentUtils::GetTextForNode(aTarget, target);

            MOZ_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("nsRDFPropertyTestNode[%p]: Retract([%s]==[%s]=>[%s])",
                    this, source, property, NS_ConvertUTF16toUTF8(target).get()));
        }

        mProcessor->RetractElement(Element(aSource, aProperty, aTarget));
    }
}


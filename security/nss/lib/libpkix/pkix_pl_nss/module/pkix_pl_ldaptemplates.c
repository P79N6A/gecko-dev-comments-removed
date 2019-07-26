



#include "pkix_pl_ldapt.h"

SEC_ASN1_MKSUB(SEC_AnyTemplate)
SEC_ASN1_MKSUB(SEC_NullTemplate)
SEC_ASN1_MKSUB(SEC_OctetStringTemplate)









const SEC_ASN1Template PKIX_PL_LDAPCrossCertPairTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(LDAPCertPair) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        SEC_ASN1_EXPLICIT | SEC_ASN1_XTRN | 0,
        offsetof(LDAPCertPair, forward), SEC_ASN1_SUB(SEC_AnyTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        SEC_ASN1_EXPLICIT | SEC_ASN1_XTRN | 1,
        offsetof(LDAPCertPair, reverse), SEC_ASN1_SUB(SEC_AnyTemplate) },
    { 0 }
};


















#define LDAPStringTemplate SEC_ASN1_SUB(SEC_OctetStringTemplate)

static const SEC_ASN1Template LDAPBindApplTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL },
    { SEC_ASN1_INTEGER, offsetof(LDAPBind, version) },
    { SEC_ASN1_LDAP_STRING, offsetof(LDAPBind, bindName) },
    { SEC_ASN1_LDAP_STRING, offsetof(LDAPBind, authentication) },
    { 0 }
};

static const SEC_ASN1Template LDAPBindTemplate[] = {
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_APPLICATION | LDAP_BIND_TYPE, 0,
        LDAPBindApplTemplate, sizeof (LDAPBind) }
};















































static const SEC_ASN1Template LDAPResultTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL },
    { SEC_ASN1_ENUMERATED, offsetof(LDAPResult, resultCode) },
    { SEC_ASN1_LDAP_STRING, offsetof(LDAPResult, matchedDN) },
    { SEC_ASN1_LDAP_STRING, offsetof(LDAPResult, errorMessage) },
    { 0 }
};

static const SEC_ASN1Template LDAPBindResponseTemplate[] = {
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_APPLICATION | LDAP_BINDRESPONSE_TYPE, 0,
        LDAPResultTemplate, sizeof (LDAPBindResponse) }
};





static const SEC_ASN1Template LDAPUnbindTemplate[] = {
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_APPLICATION | SEC_ASN1_XTRN |
        LDAP_UNBIND_TYPE , 0, SEC_ASN1_SUB(SEC_NullTemplate) }
};















#define LDAPAttributeTypeTemplate LDAPStringTemplate













#define LDAPSubstringFilterInitialTemplate LDAPStringTemplate
#define LDAPSubstringFilterAnyTemplate LDAPStringTemplate
#define LDAPSubstringFilterFinalTemplate LDAPStringTemplate

static const SEC_ASN1Template LDAPSubstringFilterChoiceTemplate[] = {
    { SEC_ASN1_CHOICE, offsetof(LDAPSubstring, selector), 0,
        sizeof (LDAPFilter) },
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
        offsetof(LDAPSubstring, item),
        LDAPSubstringFilterInitialTemplate,
        LDAP_INITIALSUBSTRING_TYPE },
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 1,
        offsetof(LDAPSubstring, item),
        LDAPSubstringFilterAnyTemplate,
        LDAP_ANYSUBSTRING_TYPE },
    { SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 2,
        offsetof(LDAPSubstring, item),
        LDAPSubstringFilterFinalTemplate,
        LDAP_FINALSUBSTRING_TYPE },
    { 0 }
};
















static const SEC_ASN1Template LDAPSubstringFilterTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof (LDAPSubstringFilter) },
    { SEC_ASN1_LDAP_STRING, offsetof(LDAPSubstringFilter, attrType) },
    { SEC_ASN1_SEQUENCE_OF, offsetof(LDAPSubstringFilter, strings),
        LDAPSubstringFilterChoiceTemplate },
    { 0 }
};

const SEC_ASN1Template LDAPFilterTemplate[]; 

static const SEC_ASN1Template LDAPSetOfFiltersTemplate[] = {
    { SEC_ASN1_SET_OF, 0, LDAPFilterTemplate }
};

static const SEC_ASN1Template LDAPAVAFilterTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof (LDAPAttributeValueAssertion) },
    { SEC_ASN1_LDAP_STRING, offsetof(LDAPAttributeValueAssertion, attrType) },
    { SEC_ASN1_OCTET_STRING, offsetof(LDAPAttributeValueAssertion, attrValue) },
    { 0 }
};

static const SEC_ASN1Template LDAPPresentFilterTemplate[] = {
    { SEC_ASN1_LDAP_STRING, offsetof(LDAPPresentFilter, attrType) }
};

#define LDAPEqualFilterTemplate LDAPAVAFilterTemplate
#define LDAPGreaterOrEqualFilterTemplate LDAPAVAFilterTemplate
#define LDAPLessOrEqualFilterTemplate LDAPAVAFilterTemplate
#define LDAPApproxMatchFilterTemplate LDAPAVAFilterTemplate

const SEC_ASN1Template LDAPFilterTemplate[] = {
    { SEC_ASN1_CHOICE, offsetof(LDAPFilter, selector), 0, sizeof(LDAPFilter) },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        LDAP_ANDFILTER_TYPE,
        offsetof(LDAPFilter, filter.andFilter.filters),
        LDAPSetOfFiltersTemplate, LDAP_ANDFILTER_TYPE },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        LDAP_ORFILTER_TYPE,
        offsetof(LDAPFilter, filter.orFilter.filters),
        LDAPSetOfFiltersTemplate, LDAP_ORFILTER_TYPE },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        LDAP_NOTFILTER_TYPE | SEC_ASN1_POINTER,
        offsetof(LDAPFilter, filter.notFilter),
        LDAPFilterTemplate, LDAP_NOTFILTER_TYPE },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        LDAP_EQUALFILTER_TYPE,
        offsetof(LDAPFilter, filter.equalFilter),
        LDAPEqualFilterTemplate, LDAP_EQUALFILTER_TYPE },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        LDAP_SUBSTRINGFILTER_TYPE, offsetof(LDAPFilter, filter.substringFilter),
        LDAPSubstringFilterTemplate, LDAP_SUBSTRINGFILTER_TYPE },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        LDAP_GREATEROREQUALFILTER_TYPE,
        offsetof(LDAPFilter, filter.greaterOrEqualFilter),
        LDAPGreaterOrEqualFilterTemplate, LDAP_GREATEROREQUALFILTER_TYPE },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        LDAP_LESSOREQUALFILTER_TYPE,
        offsetof(LDAPFilter, filter.lessOrEqualFilter),
        LDAPLessOrEqualFilterTemplate, LDAP_LESSOREQUALFILTER_TYPE },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        LDAP_PRESENTFILTER_TYPE,
        offsetof(LDAPFilter, filter.presentFilter),
        LDAPPresentFilterTemplate, LDAP_PRESENTFILTER_TYPE },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
        LDAP_APPROXMATCHFILTER_TYPE,
        offsetof(LDAPFilter, filter.approxMatchFilter),
        LDAPApproxMatchFilterTemplate, LDAP_APPROXMATCHFILTER_TYPE },
    { 0 }
};




























static const SEC_ASN1Template LDAPAttributeTemplate[] = {
    { SEC_ASN1_LDAP_STRING, 0, NULL, sizeof (SECItem) }
};

static const SEC_ASN1Template LDAPSearchApplTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL },
    { SEC_ASN1_LDAP_STRING, offsetof(LDAPSearch, baseObject) },
    { SEC_ASN1_ENUMERATED, offsetof(LDAPSearch, scope) },
    { SEC_ASN1_ENUMERATED, offsetof(LDAPSearch, derefAliases) },
    { SEC_ASN1_INTEGER, offsetof(LDAPSearch, sizeLimit) },
    { SEC_ASN1_INTEGER, offsetof(LDAPSearch, timeLimit) },
    { SEC_ASN1_BOOLEAN, offsetof(LDAPSearch, attrsOnly) },
    { SEC_ASN1_INLINE, offsetof(LDAPSearch, filter), LDAPFilterTemplate },
    { SEC_ASN1_SEQUENCE_OF, offsetof(LDAPSearch, attributes), LDAPAttributeTemplate },
    { 0 }
};

static const SEC_ASN1Template LDAPSearchTemplate[] = {
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_APPLICATION | LDAP_SEARCH_TYPE, 0,
        LDAPSearchApplTemplate, sizeof (LDAPSearch) } 
};















static const SEC_ASN1Template LDAPSearchResponseAttrTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(LDAPSearchResponseAttr) },
    { SEC_ASN1_LDAP_STRING, offsetof(LDAPSearchResponseAttr, attrType) },
    { SEC_ASN1_SET_OF | SEC_ASN1_XTRN, offsetof(LDAPSearchResponseAttr, val),
        LDAPStringTemplate },
    { 0 }
};

static const SEC_ASN1Template LDAPEntryTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL },
    { SEC_ASN1_LDAP_STRING, offsetof(LDAPSearchResponseEntry, objectName) },
    { SEC_ASN1_SEQUENCE_OF, offsetof(LDAPSearchResponseEntry, attributes),
        LDAPSearchResponseAttrTemplate },
    { 0 }
};

static const SEC_ASN1Template LDAPSearchResponseEntryTemplate[] = {
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_APPLICATION | LDAP_SEARCHRESPONSEENTRY_TYPE, 0,
        LDAPEntryTemplate, sizeof (LDAPSearchResponseEntry) }
};

static const SEC_ASN1Template LDAPSearchResponseResultTemplate[] = {
    { SEC_ASN1_APPLICATION | LDAP_SEARCHRESPONSERESULT_TYPE, 0,
        LDAPResultTemplate, sizeof (LDAPSearchResponseResult) }
};






static const SEC_ASN1Template LDAPAbandonTemplate[] = {
    { SEC_ASN1_INTEGER, offsetof(LDAPAbandonRequest, messageID) }
};

static const SEC_ASN1Template LDAPAbandonRequestTemplate[] = {
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_APPLICATION | LDAP_ABANDONREQUEST_TYPE, 0,
        LDAPAbandonTemplate, sizeof (LDAPAbandonRequest) }
};




















static const SEC_ASN1Template LDAPMessageProtocolOpTemplate[] = {
    { SEC_ASN1_CHOICE, offsetof(LDAPProtocolOp, selector), 0, sizeof (LDAPProtocolOp) },
    { SEC_ASN1_INLINE, offsetof(LDAPProtocolOp, op.bindMsg),
        LDAPBindTemplate, LDAP_BIND_TYPE },
    { SEC_ASN1_INLINE, offsetof(LDAPProtocolOp, op.bindResponseMsg),
        LDAPBindResponseTemplate, LDAP_BINDRESPONSE_TYPE },
    { SEC_ASN1_INLINE, offsetof(LDAPProtocolOp, op.unbindMsg),
        LDAPUnbindTemplate, LDAP_UNBIND_TYPE },
    { SEC_ASN1_INLINE, offsetof(LDAPProtocolOp, op.searchMsg),
        LDAPSearchTemplate, LDAP_SEARCH_TYPE },
    { SEC_ASN1_INLINE, offsetof(LDAPProtocolOp, op.searchResponseEntryMsg),
        LDAPSearchResponseEntryTemplate, LDAP_SEARCHRESPONSEENTRY_TYPE },
    { SEC_ASN1_INLINE, offsetof(LDAPProtocolOp, op.searchResponseResultMsg),
        LDAPSearchResponseResultTemplate, LDAP_SEARCHRESPONSERESULT_TYPE },
    { SEC_ASN1_INLINE, offsetof(LDAPProtocolOp, op.abandonRequestMsg),
        LDAPAbandonRequestTemplate, LDAP_ABANDONREQUEST_TYPE },
    { 0 }
};

const SEC_ASN1Template PKIX_PL_LDAPMessageTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL },
    { SEC_ASN1_INTEGER, offsetof(LDAPMessage, messageID) },
    { SEC_ASN1_INLINE, offsetof(LDAPMessage, protocolOp),
        LDAPMessageProtocolOpTemplate },
    { 0 }
};




SEC_ASN1_CHOOSER_IMPLEMENT(PKIX_PL_LDAPMessageTemplate)

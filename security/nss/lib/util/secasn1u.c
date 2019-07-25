









































#include "secasn1.h"










int
SEC_ASN1LengthLength (unsigned long len)
{
    int lenlen = 1;

    if (len > 0x7f) {
	do {
	    lenlen++;
	    len >>= 8;
	} while (len);
    }

    return lenlen;
}



















const SEC_ASN1Template *
SEC_ASN1GetSubtemplate (const SEC_ASN1Template *theTemplate, void *thing,
			PRBool encoding)
{
    const SEC_ASN1Template *subt = NULL;

    PORT_Assert (theTemplate->sub != NULL);
    if (theTemplate->sub != NULL) {
	if (theTemplate->kind & SEC_ASN1_DYNAMIC) {
	    SEC_ASN1TemplateChooserPtr chooserp;

	    chooserp = *(SEC_ASN1TemplateChooserPtr *) theTemplate->sub;
	    if (chooserp) {
		if (thing != NULL)
		    thing = (char *)thing - theTemplate->offset;
		subt = (* chooserp)(thing, encoding);
	    }
	} else {
	    subt = (SEC_ASN1Template*)theTemplate->sub;
	}
    }
    return subt;
}

PRBool SEC_ASN1IsTemplateSimple(const SEC_ASN1Template *theTemplate)
{
    if (!theTemplate) {
	return PR_TRUE; 
    }
    

    if (! (theTemplate->kind & (~SEC_ASN1_TAGNUM_MASK))) {
	return PR_TRUE; 
    }
    if (!(theTemplate->kind & SEC_ASN1_CHOICE)) {
	return PR_FALSE; 
    }
    while (++theTemplate && theTemplate->kind) {
	if (theTemplate->kind & (~SEC_ASN1_TAGNUM_MASK)) {
	    return PR_FALSE; 
	}
    }
    return PR_TRUE; 
}


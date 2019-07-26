







#include "cmslocal.h"

#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "prtime.h"
#include "secerr.h"


















NSSCMSAttribute *
NSS_CMSAttribute_Create(PLArenaPool *poolp, SECOidTag oidtag, SECItem *value, PRBool encoded)
{
    NSSCMSAttribute *attr;
    SECItem *copiedvalue;
    void *mark;

    PORT_Assert (poolp != NULL);

    mark = PORT_ArenaMark (poolp);

    attr = (NSSCMSAttribute *)PORT_ArenaZAlloc(poolp, sizeof(NSSCMSAttribute));
    if (attr == NULL)
	goto loser;

    attr->typeTag = SECOID_FindOIDByTag(oidtag);
    if (attr->typeTag == NULL)
	goto loser;

    if (SECITEM_CopyItem(poolp, &(attr->type), &(attr->typeTag->oid)) != SECSuccess)
	goto loser;

    if (value != NULL) {
	if ((copiedvalue = SECITEM_ArenaDupItem(poolp, value)) == NULL)
	    goto loser;

	if (NSS_CMSArray_Add(poolp, (void ***)&(attr->values), (void *)copiedvalue) != SECSuccess)
	    goto loser;
    }

    attr->encoded = encoded;

    PORT_ArenaUnmark (poolp, mark);

    return attr;

loser:
    PORT_Assert (mark != NULL);
    PORT_ArenaRelease (poolp, mark);
    return NULL;
}




SECStatus
NSS_CMSAttribute_AddValue(PLArenaPool *poolp, NSSCMSAttribute *attr, SECItem *value)
{
    SECItem *copiedvalue;
    void *mark;

    PORT_Assert (poolp != NULL);

    mark = PORT_ArenaMark(poolp);

    if (value == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	goto loser;
    }

    if ((copiedvalue = SECITEM_ArenaDupItem(poolp, value)) == NULL)
	goto loser;

    if (NSS_CMSArray_Add(poolp, (void ***)&(attr->values), (void *)copiedvalue) != SECSuccess)
	goto loser;

    PORT_ArenaUnmark(poolp, mark);
    return SECSuccess;

loser:
    PORT_Assert (mark != NULL);
    PORT_ArenaRelease (poolp, mark);
    return SECFailure;
}




SECOidTag
NSS_CMSAttribute_GetType(NSSCMSAttribute *attr)
{
    SECOidData *typetag;

    typetag = SECOID_FindOID(&(attr->type));
    if (typetag == NULL)
	return SEC_OID_UNKNOWN;

    return typetag->offset;
}








SECItem *
NSS_CMSAttribute_GetValue(NSSCMSAttribute *attr)
{
    SECItem *value;

    if (attr == NULL)
	return NULL;

    value = attr->values[0];

    if (value == NULL || value->data == NULL || value->len == 0)
	return NULL;

    if (attr->values[1] != NULL)
	return NULL;

    return value;
}




PRBool
NSS_CMSAttribute_CompareValue(NSSCMSAttribute *attr, SECItem *av)
{
    SECItem *value;
    
    if (attr == NULL)
	return PR_FALSE;

    value = NSS_CMSAttribute_GetValue(attr);

    return (value != NULL && value->len == av->len &&
	PORT_Memcmp (value->data, av->data, value->len) == 0);
}










static const SEC_ASN1Template *
cms_attr_choose_attr_value_template(void *src_or_dest, PRBool encoding)
{
    const SEC_ASN1Template *theTemplate;
    NSSCMSAttribute *attribute;
    SECOidData *oiddata;
    PRBool encoded;

    PORT_Assert (src_or_dest != NULL);
    if (src_or_dest == NULL)
	return NULL;

    attribute = (NSSCMSAttribute *)src_or_dest;

    if (encoding && (!attribute->values || !attribute->values[0] ||
        attribute->encoded)) {
        

        return SEC_ASN1_GET(SEC_AnyTemplate);
    }

    
    oiddata = attribute->typeTag;
    if (oiddata == NULL) {
	oiddata = SECOID_FindOID(&attribute->type);
	attribute->typeTag = oiddata;
    }

    if (oiddata == NULL) {
	
	encoded = PR_TRUE;
	theTemplate = SEC_ASN1_GET(SEC_AnyTemplate);
    } else {
	switch (oiddata->offset) {
	case SEC_OID_PKCS9_SMIME_CAPABILITIES:
	case SEC_OID_SMIME_ENCRYPTION_KEY_PREFERENCE:
	    
	default:
	    
	    encoded = PR_TRUE;
	    theTemplate = SEC_ASN1_GET(SEC_AnyTemplate);
	    break;
	    
	case SEC_OID_PKCS9_EMAIL_ADDRESS:
	case SEC_OID_RFC1274_MAIL:
	case SEC_OID_PKCS9_UNSTRUCTURED_NAME:
	    encoded = PR_FALSE;
	    theTemplate = SEC_ASN1_GET(SEC_IA5StringTemplate);
	    break;
	case SEC_OID_PKCS9_CONTENT_TYPE:
	    encoded = PR_FALSE;
	    theTemplate = SEC_ASN1_GET(SEC_ObjectIDTemplate);
	    break;
	case SEC_OID_PKCS9_MESSAGE_DIGEST:
	    encoded = PR_FALSE;
	    theTemplate = SEC_ASN1_GET(SEC_OctetStringTemplate);
	    break;
	case SEC_OID_PKCS9_SIGNING_TIME:
	    encoded = PR_FALSE;
	    theTemplate = SEC_ASN1_GET(CERT_TimeChoiceTemplate);
	    break;
	  
	}
    }

    if (encoding) {
	






	PORT_Assert (!encoded);
    } else {
	



	attribute->encoded = encoded;
    }
    return theTemplate;
}

static const SEC_ASN1TemplateChooserPtr cms_attr_chooser
	= cms_attr_choose_attr_value_template;

const SEC_ASN1Template nss_cms_attribute_template[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(NSSCMSAttribute) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(NSSCMSAttribute,type) },
    { SEC_ASN1_DYNAMIC | SEC_ASN1_SET_OF,
	  offsetof(NSSCMSAttribute,values),
	  &cms_attr_chooser },
    { 0 }
};

const SEC_ASN1Template nss_cms_set_of_attribute_template[] = {
    { SEC_ASN1_SET_OF, 0, nss_cms_attribute_template },
};














SECItem *
NSS_CMSAttributeArray_Encode(PLArenaPool *poolp, NSSCMSAttribute ***attrs, SECItem *dest)
{
    return SEC_ASN1EncodeItem (poolp, dest, (void *)attrs, nss_cms_set_of_attribute_template);
}








SECStatus
NSS_CMSAttributeArray_Reorder(NSSCMSAttribute **attrs)
{
    return NSS_CMSArray_SortByDER((void **)attrs, nss_cms_attribute_template, NULL);
}









NSSCMSAttribute *
NSS_CMSAttributeArray_FindAttrByOidTag(NSSCMSAttribute **attrs, SECOidTag oidtag, PRBool only)
{
    SECOidData *oid;
    NSSCMSAttribute *attr1, *attr2;

    if (attrs == NULL)
	return NULL;

    oid = SECOID_FindOIDByTag(oidtag);
    if (oid == NULL)
	return NULL;

    while ((attr1 = *attrs++) != NULL) {
	if (attr1->type.len == oid->oid.len && PORT_Memcmp (attr1->type.data,
							    oid->oid.data,
							    oid->oid.len) == 0)
	    break;
    }

    if (attr1 == NULL)
	return NULL;

    if (!only)
	return attr1;

    while ((attr2 = *attrs++) != NULL) {
	if (attr2->type.len == oid->oid.len && PORT_Memcmp (attr2->type.data,
							    oid->oid.data,
							    oid->oid.len) == 0)
	    break;
    }

    if (attr2 != NULL)
	return NULL;

    return attr1;
}





SECStatus
NSS_CMSAttributeArray_AddAttr(PLArenaPool *poolp, NSSCMSAttribute ***attrs, NSSCMSAttribute *attr)
{
    NSSCMSAttribute *oattr;
    void *mark;
    SECOidTag type;

    mark = PORT_ArenaMark(poolp);

    
    type = NSS_CMSAttribute_GetType(attr);

    
    oattr = NSS_CMSAttributeArray_FindAttrByOidTag(*attrs, type, PR_FALSE);
    PORT_Assert (oattr == NULL);
    if (oattr != NULL)
	goto loser;	

    
    if (NSS_CMSArray_Add(poolp, (void ***)attrs, (void *)attr) != SECSuccess)
	goto loser;

    PORT_ArenaUnmark(poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease(poolp, mark);
    return SECFailure;
}




SECStatus
NSS_CMSAttributeArray_SetAttr(PLArenaPool *poolp, NSSCMSAttribute ***attrs, SECOidTag type, SECItem *value, PRBool encoded)
{
    NSSCMSAttribute *attr;
    void *mark;

    mark = PORT_ArenaMark(poolp);

    
    attr = NSS_CMSAttributeArray_FindAttrByOidTag(*attrs, type, PR_FALSE);
    if (attr == NULL) {
	
	attr = NSS_CMSAttribute_Create(poolp, type, value, encoded);
	if (attr == NULL)
	    goto loser;
	
	if (NSS_CMSArray_Add(poolp, (void ***)attrs, (void *)attr) != SECSuccess)
	    goto loser;
    } else {
	
	
	attr->values[0] = value;
	attr->encoded = encoded;
    }

    PORT_ArenaUnmark (poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease (poolp, mark);
    return SECFailure;
}


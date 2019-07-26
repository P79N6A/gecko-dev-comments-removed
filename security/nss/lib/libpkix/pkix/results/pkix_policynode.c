









#include "pkix_policynode.h"



































PKIX_Error *
pkix_PolicyNode_GetChildrenMutable(
        PKIX_PolicyNode *node,
        PKIX_List **pChildren,  
        void *plContext)
{

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_GetChildrenMutable");

        PKIX_NULLCHECK_TWO(node, pChildren);

        PKIX_INCREF(node->children);

        *pChildren = node->children;

cleanup:
        PKIX_RETURN(CERTPOLICYNODE);
}







































PKIX_Error *
pkix_PolicyNode_Create(
        PKIX_PL_OID *validPolicy,
        PKIX_List *qualifierSet,
        PKIX_Boolean criticality,
        PKIX_List *expectedPolicySet,
        PKIX_PolicyNode **pObject,
        void *plContext)
{
        PKIX_PolicyNode *node = NULL;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_Create");

        PKIX_NULLCHECK_THREE(validPolicy, expectedPolicySet, pObject);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_CERTPOLICYNODE_TYPE,
                sizeof (PKIX_PolicyNode),
                (PKIX_PL_Object **)&node,
                plContext),
                PKIX_COULDNOTCREATEPOLICYNODEOBJECT);

        PKIX_INCREF(validPolicy);
        node->validPolicy = validPolicy;

        PKIX_INCREF(qualifierSet);
        node->qualifierSet = qualifierSet;
        if (qualifierSet) {
                PKIX_CHECK(PKIX_List_SetImmutable(qualifierSet, plContext),
                        PKIX_LISTSETIMMUTABLEFAILED);
        }

        node->criticality = criticality;

        PKIX_INCREF(expectedPolicySet);
        node->expectedPolicySet = expectedPolicySet;
        PKIX_CHECK(PKIX_List_SetImmutable(expectedPolicySet, plContext),
                PKIX_LISTSETIMMUTABLEFAILED);

        node->parent = NULL;
        node->children = NULL;
        node->depth = 0;

        *pObject = node;
        node = NULL;

cleanup:

        PKIX_DECREF(node);

        PKIX_RETURN(CERTPOLICYNODE);
}































PKIX_Error *
pkix_PolicyNode_AddToParent(
        PKIX_PolicyNode *parentNode,
        PKIX_PolicyNode *child,
        void *plContext)
{
        PKIX_List *listOfChildren = NULL;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_AddToParent");

        PKIX_NULLCHECK_TWO(parentNode, child);

        listOfChildren = parentNode->children;
        if (listOfChildren == NULL) {
                PKIX_CHECK(PKIX_List_Create(&listOfChildren, plContext),
                        PKIX_LISTCREATEFAILED);
                parentNode->children = listOfChildren;
        }

        





        child->parent = parentNode;

        child->depth = 1 + (parentNode->depth);

        PKIX_CHECK(PKIX_List_AppendItem
                (listOfChildren, (PKIX_PL_Object *)child, plContext),
                PKIX_COULDNOTAPPENDCHILDTOPARENTSPOLICYNODELIST);

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)parentNode, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)child, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

cleanup:

        PKIX_RETURN(CERTPOLICYNODE);
}

































PKIX_Error *
pkix_PolicyNode_Prune(
        PKIX_PolicyNode *node,
        PKIX_UInt32 height,
        PKIX_Boolean *pDelete,
        void *plContext)
{
        PKIX_Boolean childless = PKIX_FALSE;
        PKIX_Boolean shouldBePruned = PKIX_FALSE;
        PKIX_UInt32 listSize = 0;
        PKIX_UInt32 listIndex = 0;
        PKIX_PolicyNode *candidate = NULL;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_Prune");

        PKIX_NULLCHECK_TWO(node, pDelete);

        
        if (height == 0) {
                goto cleanup;
        }

        
        if (!(node->children)) {
                childless = PKIX_TRUE;
                goto cleanup;
        }

        



        if (height > 1) {
                PKIX_CHECK(PKIX_List_GetLength
                        (node->children, &listSize, plContext),
                        PKIX_LISTGETLENGTHFAILED);
                








                for (listIndex = listSize; listIndex > 0; listIndex--) {
                        PKIX_CHECK(PKIX_List_GetItem
                                (node->children,
                                (listIndex - 1),
                                (PKIX_PL_Object **)&candidate,
                                plContext),
                                PKIX_LISTGETITEMFAILED);

                        PKIX_CHECK(pkix_PolicyNode_Prune
                                (candidate,
                                height - 1,
                                &shouldBePruned,
                                plContext),
                                PKIX_POLICYNODEPRUNEFAILED);

                        if (shouldBePruned == PKIX_TRUE) {
                                PKIX_CHECK(PKIX_List_DeleteItem
                                        (node->children,
                                        (listIndex - 1),
                                        plContext),
                                        PKIX_LISTDELETEITEMFAILED);
                        }

                        PKIX_DECREF(candidate);
                }
        }

        
        PKIX_CHECK(PKIX_List_GetLength
                (node->children, &listSize, plContext),
                PKIX_LISTGETLENGTHFAILED);
        if (listSize == 0) {
                childless = PKIX_TRUE;
        }

        



        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)node, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

cleanup:
        *pDelete = childless;

        PKIX_DECREF(candidate);

        PKIX_RETURN(CERTPOLICYNODE);
}
























PKIX_Error *
pkix_SinglePolicyNode_ToString(
        PKIX_PolicyNode *node,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_String *fmtString = NULL;
        PKIX_PL_String *validString = NULL;
        PKIX_PL_String *qualifierString = NULL;
        PKIX_PL_String *criticalityString = NULL;
        PKIX_PL_String *expectedString = NULL;
        PKIX_PL_String *outString = NULL;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_SinglePolicyNode_ToString");
        PKIX_NULLCHECK_TWO(node, pString);
        PKIX_NULLCHECK_TWO(node->validPolicy, node->expectedPolicySet);

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII,
                "{%s,%s,%s,%s,%d}",
                0,
                &fmtString,
                plContext),
                PKIX_CANTCREATESTRING);

        PKIX_CHECK(PKIX_PL_Object_ToString
                ((PKIX_PL_Object *)(node->validPolicy),
                &validString,
                plContext),
                PKIX_OIDTOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_Object_ToString
                ((PKIX_PL_Object *)(node->expectedPolicySet),
                &expectedString,
                plContext),
                PKIX_LISTTOSTRINGFAILED);

        if (node->qualifierSet) {
                PKIX_CHECK(PKIX_PL_Object_ToString
                        ((PKIX_PL_Object *)(node->qualifierSet),
                        &qualifierString,
                        plContext),
                        PKIX_LISTTOSTRINGFAILED);
        } else {
                PKIX_CHECK(PKIX_PL_String_Create
                        (PKIX_ESCASCII,
                        "{}",
                        0,
                        &qualifierString,
                        plContext),
                        PKIX_CANTCREATESTRING);
        }

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII,
                (node->criticality)?"Critical":"Not Critical",
                0,
                &criticalityString,
                plContext),
                PKIX_CANTCREATESTRING);

        PKIX_CHECK(PKIX_PL_Sprintf
                (&outString,
                plContext,
                fmtString,
                validString,
                qualifierString,
                criticalityString,
                expectedString,
                node->depth),
                PKIX_SPRINTFFAILED);

        *pString = outString;

cleanup:

        PKIX_DECREF(fmtString);
        PKIX_DECREF(validString);
        PKIX_DECREF(qualifierString);
        PKIX_DECREF(criticalityString);
        PKIX_DECREF(expectedString);
        PKIX_RETURN(CERTPOLICYNODE);
}





























static PKIX_Error *
pkix_PolicyNode_ToString_Helper(
        PKIX_PolicyNode *rootNode,
        PKIX_PL_String *indent,
        PKIX_PL_String **pTreeString,
        void *plContext)
{
        PKIX_PL_String *nextIndentFormat = NULL;
        PKIX_PL_String *thisNodeFormat = NULL;
        PKIX_PL_String *childrenFormat = NULL;
        PKIX_PL_String *nextIndentString = NULL;
        PKIX_PL_String *resultString = NULL;
        PKIX_PL_String *thisItemString = NULL;
        PKIX_PL_String *childString = NULL;
        PKIX_PolicyNode *childNode = NULL;
        PKIX_UInt32 numberOfChildren = 0;
        PKIX_UInt32 childIndex = 0;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_ToString_Helper");

        PKIX_NULLCHECK_TWO(rootNode, pTreeString);

        
        PKIX_CHECK(pkix_SinglePolicyNode_ToString
                (rootNode, &thisItemString, plContext),
                PKIX_ERRORINSINGLEPOLICYNODETOSTRING);

        if (indent) {
                PKIX_CHECK(PKIX_PL_String_Create
                        (PKIX_ESCASCII,
                        "%s%s",
                        0,
                        &thisNodeFormat,
                        plContext),
                        PKIX_ERRORCREATINGFORMATSTRING);

                PKIX_CHECK(PKIX_PL_Sprintf
                        (&resultString,
                        plContext,
                        thisNodeFormat,
                        indent,
                        thisItemString),
                        PKIX_ERRORINSPRINTF);
        } else {
                PKIX_CHECK(PKIX_PL_String_Create
                        (PKIX_ESCASCII,
                        "%s",
                        0,
                        &thisNodeFormat,
                        plContext),
                        PKIX_ERRORCREATINGFORMATSTRING);

                PKIX_CHECK(PKIX_PL_Sprintf
                        (&resultString,
                        plContext,
                        thisNodeFormat,
                        thisItemString),
                        PKIX_ERRORINSPRINTF);
        }

        PKIX_DECREF(thisItemString);
        thisItemString = resultString;

        
        if (rootNode->children) {
                PKIX_CHECK(PKIX_List_GetLength
                        (rootNode->children, &numberOfChildren, plContext),
                        PKIX_LISTGETLENGTHFAILED);
        }

        if (numberOfChildren != 0) {
                




                
                if (indent) {
                        PKIX_CHECK(PKIX_PL_String_Create
                                (PKIX_ESCASCII,
                                "%s. ",
                                0,
                                &nextIndentFormat,
                                plContext),
                                PKIX_ERRORCREATINGFORMATSTRING);

                        PKIX_CHECK(PKIX_PL_Sprintf
                                (&nextIndentString,
                                plContext,
                                nextIndentFormat,
                                indent),
                                PKIX_ERRORINSPRINTF);
                } else {
                        PKIX_CHECK(PKIX_PL_String_Create
                                (PKIX_ESCASCII,
                                ". ",
                                0,
                                &nextIndentString,
                                plContext),
                                PKIX_ERRORCREATINGINDENTSTRING);
                }

                
                PKIX_CHECK(PKIX_PL_String_Create
                        (PKIX_ESCASCII,
                        "%s\n%s",
                        0,
                        &childrenFormat,
                        plContext),
                        PKIX_ERRORCREATINGFORMATSTRING);

                for (childIndex = 0;
                        childIndex < numberOfChildren;
                        childIndex++) {
                        PKIX_CHECK(PKIX_List_GetItem
                                (rootNode->children,
                                childIndex,
                                (PKIX_PL_Object **)&childNode,
                                plContext),
                                PKIX_LISTGETITEMFAILED);

                        PKIX_CHECK(pkix_PolicyNode_ToString_Helper
                                (childNode,
                                nextIndentString,
                                &childString,
                                plContext),
                                PKIX_ERRORCREATINGCHILDSTRING);


                        PKIX_CHECK(PKIX_PL_Sprintf
                                (&resultString,
                                plContext,
                                childrenFormat,
                                thisItemString,
                                childString),
                        PKIX_ERRORINSPRINTF);

                        PKIX_DECREF(childNode);
                        PKIX_DECREF(childString);
                        PKIX_DECREF(thisItemString);

                        thisItemString = resultString;
                }
        }

        *pTreeString = thisItemString;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(thisItemString);
        }

        PKIX_DECREF(nextIndentFormat);
        PKIX_DECREF(thisNodeFormat);
        PKIX_DECREF(childrenFormat);
        PKIX_DECREF(nextIndentString);
        PKIX_DECREF(childString);
        PKIX_DECREF(childNode);

        PKIX_RETURN(CERTPOLICYNODE);
}





static PKIX_Error *
pkix_PolicyNode_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pTreeString,
        void *plContext)
{
        PKIX_PolicyNode *rootNode = NULL;
        PKIX_PL_String *resultString = NULL;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_ToString");

        PKIX_NULLCHECK_TWO(object, pTreeString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERTPOLICYNODE_TYPE, plContext),
                PKIX_OBJECTNOTPOLICYNODE);

        rootNode = (PKIX_PolicyNode *)object;

        PKIX_CHECK(pkix_PolicyNode_ToString_Helper
                (rootNode, NULL, &resultString, plContext),
                PKIX_ERRORCREATINGSUBTREESTRING);

        *pTreeString = resultString;

cleanup:

        PKIX_RETURN(CERTPOLICYNODE);
}





static PKIX_Error *
pkix_PolicyNode_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PolicyNode *node = NULL;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_Destroy");

        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERTPOLICYNODE_TYPE, plContext),
                PKIX_OBJECTNOTPOLICYNODE);

        node = (PKIX_PolicyNode*)object;

        node->criticality = PKIX_FALSE;
        PKIX_DECREF(node->validPolicy);
        PKIX_DECREF(node->qualifierSet);
        PKIX_DECREF(node->expectedPolicySet);
        PKIX_DECREF(node->children);

        



        node->parent = NULL;
        node->depth = 0;

cleanup:

        PKIX_RETURN(CERTPOLICYNODE);
}
























static PKIX_Error *
pkix_SinglePolicyNode_Hashcode(
        PKIX_PolicyNode *node,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_UInt32 componentHash = 0;
        PKIX_UInt32 nodeHash = 0;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_SinglePolicyNode_Hashcode");
        PKIX_NULLCHECK_TWO(node, pHashcode);
        PKIX_NULLCHECK_TWO(node->validPolicy, node->expectedPolicySet);

        PKIX_HASHCODE
                (node->qualifierSet,
                &nodeHash,
                plContext,
                PKIX_FAILUREHASHINGLISTQUALIFIERSET);

        if (PKIX_TRUE == (node->criticality)) {
                nodeHash = 31*nodeHash + 0xff;
        } else {
                nodeHash = 31*nodeHash + 0x00;
        }

        PKIX_CHECK(PKIX_PL_Object_Hashcode
                ((PKIX_PL_Object *)node->validPolicy,
                &componentHash,
                plContext),
                PKIX_FAILUREHASHINGOIDVALIDPOLICY);

        nodeHash = 31*nodeHash + componentHash;

        PKIX_CHECK(PKIX_PL_Object_Hashcode
                ((PKIX_PL_Object *)node->expectedPolicySet,
                &componentHash,
                plContext),
                PKIX_FAILUREHASHINGLISTEXPECTEDPOLICYSET);

        nodeHash = 31*nodeHash + componentHash;

        *pHashcode = nodeHash;

cleanup:

        PKIX_RETURN(CERTPOLICYNODE);
}





static PKIX_Error *
pkix_PolicyNode_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PolicyNode *node = NULL;
        PKIX_UInt32 childrenHash = 0;
        PKIX_UInt32 nodeHash = 0;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_CERTPOLICYNODE_TYPE, plContext),
                PKIX_OBJECTNOTPOLICYNODE);

        node = (PKIX_PolicyNode *)object;

        PKIX_CHECK(pkix_SinglePolicyNode_Hashcode
                (node, &nodeHash, plContext),
                PKIX_SINGLEPOLICYNODEHASHCODEFAILED);

        nodeHash = 31*nodeHash + (PKIX_UInt32)(node->parent);

        PKIX_HASHCODE
                (node->children,
                &childrenHash,
                plContext,
                PKIX_OBJECTHASHCODEFAILED);

        nodeHash = 31*nodeHash + childrenHash;

        *pHashcode = nodeHash;

cleanup:

        PKIX_RETURN(CERTPOLICYNODE);
}



























static PKIX_Error *
pkix_SinglePolicyNode_Equals(
        PKIX_PolicyNode *firstPN,
        PKIX_PolicyNode *secondPN,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_Boolean compResult = PKIX_FALSE;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_SinglePolicyNode_Equals");
        PKIX_NULLCHECK_THREE(firstPN, secondPN, pResult);

        
        if (firstPN == secondPN) {
                compResult = PKIX_TRUE;
                goto cleanup;
        }

        



        if ((firstPN->criticality) != (secondPN->criticality)) {
                goto cleanup;
        }
        if ((firstPN->depth) != (secondPN->depth)) {
                goto cleanup;
        }

        PKIX_EQUALS
                (firstPN->qualifierSet,
                secondPN->qualifierSet,
                &compResult,
                plContext,
                PKIX_OBJECTEQUALSFAILED);

        if (compResult == PKIX_FALSE) {
                goto cleanup;
        }

        
        PKIX_NULLCHECK_TWO(firstPN->validPolicy, secondPN->validPolicy);

        PKIX_EQUALS
                (firstPN->validPolicy,
                secondPN->validPolicy,
                &compResult,
                plContext,
                PKIX_OBJECTEQUALSFAILED);

        if (compResult == PKIX_FALSE) {
                goto cleanup;
        }

        
        PKIX_NULLCHECK_TWO
                (firstPN->expectedPolicySet, secondPN->expectedPolicySet);

        PKIX_EQUALS
                (firstPN->expectedPolicySet,
                secondPN->expectedPolicySet,
                &compResult,
                plContext,
                PKIX_OBJECTEQUALSFAILEDONEXPECTEDPOLICYSETS);

cleanup:

        *pResult = compResult;

        PKIX_RETURN(CERTPOLICYNODE);
}





static PKIX_Error *
pkix_PolicyNode_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PolicyNode *firstPN = NULL;
        PKIX_PolicyNode *secondPN = NULL;
        PKIX_UInt32 secondType;
        PKIX_Boolean compResult = PKIX_FALSE;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType
                (firstObject, PKIX_CERTPOLICYNODE_TYPE, plContext),
                PKIX_FIRSTOBJECTNOTPOLICYNODE);

        



        if (firstObject == secondObject){
                compResult = PKIX_TRUE;
                goto cleanup;
        }

        



        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObject, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);

        if (secondType != PKIX_CERTPOLICYNODE_TYPE) {
                goto cleanup;
        }

        



        firstPN = (PKIX_PolicyNode *)firstObject;
        secondPN = (PKIX_PolicyNode *)secondObject;

        






        PKIX_EQUALS
                (firstPN->children,
                secondPN->children,
                &compResult,
                plContext,
                PKIX_OBJECTEQUALSFAILEDONCHILDREN);

        if (compResult == PKIX_FALSE) {
                goto cleanup;
        }

        PKIX_CHECK(pkix_SinglePolicyNode_Equals
                (firstPN, secondPN, &compResult, plContext),
                PKIX_SINGLEPOLICYNODEEQUALSFAILED);

cleanup:

        *pResult = compResult;

        PKIX_RETURN(CERTPOLICYNODE);
}

































static PKIX_Error *
pkix_PolicyNode_DuplicateHelper(
        PKIX_PolicyNode *original,
        PKIX_PolicyNode *parent,
        PKIX_PolicyNode **pNewNode,
        void *plContext)
{
        PKIX_UInt32 numChildren = 0;
        PKIX_UInt32 childIndex = 0;
        PKIX_List *children = NULL; 
        PKIX_PolicyNode *copy = NULL;
        PKIX_PolicyNode *child = NULL;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_DuplicateHelper");

        PKIX_NULLCHECK_THREE
                (original, original->validPolicy, original->expectedPolicySet);

        




        PKIX_CHECK(pkix_PolicyNode_Create
                (original->validPolicy,
                original->qualifierSet,
                original->criticality,
                original->expectedPolicySet,
                &copy,
                plContext),
                PKIX_POLICYNODECREATEFAILED);

        if (parent) {
                PKIX_CHECK(pkix_PolicyNode_AddToParent(parent, copy, plContext),
                        PKIX_POLICYNODEADDTOPARENTFAILED);
        }

        
        children = original->children;

        if (children) {
            PKIX_CHECK(PKIX_List_GetLength(children, &numChildren, plContext),
                PKIX_LISTGETLENGTHFAILED);
        }

        for (childIndex = 0; childIndex < numChildren; childIndex++) {
                PKIX_CHECK(PKIX_List_GetItem
                        (children,
                        childIndex,
                        (PKIX_PL_Object **)&child,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(pkix_PolicyNode_DuplicateHelper
                        (child, copy, NULL, plContext),
                        PKIX_POLICYNODEDUPLICATEHELPERFAILED);

                PKIX_DECREF(child);
        }

        if (pNewNode) {
                *pNewNode = copy;
                copy = NULL; 
        }

cleanup:
        PKIX_DECREF(copy);
        PKIX_DECREF(child);

        PKIX_RETURN(CERTPOLICYNODE);
}





static PKIX_Error *
pkix_PolicyNode_Duplicate(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext)
{
        PKIX_PolicyNode *original = NULL;
        PKIX_PolicyNode *copy = NULL;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_Duplicate");

        PKIX_NULLCHECK_TWO(object, pNewObject);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_CERTPOLICYNODE_TYPE, plContext),
                PKIX_OBJECTNOTPOLICYNODE);

        original = (PKIX_PolicyNode *)object;

        PKIX_CHECK(pkix_PolicyNode_DuplicateHelper
                (original, NULL, &copy, plContext),
                PKIX_POLICYNODEDUPLICATEHELPERFAILED);

        *pNewObject = (PKIX_PL_Object *)copy;

cleanup:

        PKIX_RETURN(CERTPOLICYNODE);
}















PKIX_Error *
pkix_PolicyNode_RegisterSelf(void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(CERTPOLICYNODE, "pkix_PolicyNode_RegisterSelf");

        entry.description = "PolicyNode";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PolicyNode);
        entry.destructor = pkix_PolicyNode_Destroy;
        entry.equalsFunction = pkix_PolicyNode_Equals;
        entry.hashcodeFunction = pkix_PolicyNode_Hashcode;
        entry.toStringFunction = pkix_PolicyNode_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_PolicyNode_Duplicate;

        systemClasses[PKIX_CERTPOLICYNODE_TYPE] = entry;

        PKIX_RETURN(CERTPOLICYNODE);
}








PKIX_Error *
PKIX_PolicyNode_GetChildren(
        PKIX_PolicyNode *node,
        PKIX_List **pChildren,  
        void *plContext)
{
        PKIX_List *children = NULL;

        PKIX_ENTER(CERTPOLICYNODE, "PKIX_PolicyNode_GetChildren");

        PKIX_NULLCHECK_TWO(node, pChildren);

        PKIX_INCREF(node->children);
        children = node->children;

        if (!children) {
                PKIX_CHECK(PKIX_List_Create(&children, plContext),
                        PKIX_LISTCREATEFAILED);
        }

        PKIX_CHECK(PKIX_List_SetImmutable(children, plContext),
                PKIX_LISTSETIMMUTABLEFAILED);

        *pChildren = children;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(children);
        }

        PKIX_RETURN(CERTPOLICYNODE);
}





PKIX_Error *
PKIX_PolicyNode_GetParent(
        PKIX_PolicyNode *node,
        PKIX_PolicyNode **pParent,
        void *plContext)
{

        PKIX_ENTER(CERTPOLICYNODE, "PKIX_PolicyNode_GetParent");

        PKIX_NULLCHECK_TWO(node, pParent);

        PKIX_INCREF(node->parent);
        *pParent = node->parent;

cleanup:
        PKIX_RETURN(CERTPOLICYNODE);
}





PKIX_Error *
PKIX_PolicyNode_GetValidPolicy(
        PKIX_PolicyNode *node,
        PKIX_PL_OID **pValidPolicy,
        void *plContext)
{

        PKIX_ENTER(CERTPOLICYNODE, "PKIX_PolicyNode_GetValidPolicy");

        PKIX_NULLCHECK_TWO(node, pValidPolicy);

        PKIX_INCREF(node->validPolicy);
        *pValidPolicy = node->validPolicy;

cleanup:
        PKIX_RETURN(CERTPOLICYNODE);
}





PKIX_Error *
PKIX_PolicyNode_GetPolicyQualifiers(
        PKIX_PolicyNode *node,
        PKIX_List **pQualifiers,  
        void *plContext)
{
        PKIX_List *qualifiers = NULL;

        PKIX_ENTER(CERTPOLICYNODE, "PKIX_PolicyNode_GetPolicyQualifiers");

        PKIX_NULLCHECK_TWO(node, pQualifiers);

        PKIX_INCREF(node->qualifierSet);
        qualifiers = node->qualifierSet;

        if (!qualifiers) {
                PKIX_CHECK(PKIX_List_Create(&qualifiers, plContext),
                        PKIX_LISTCREATEFAILED);
        }

        PKIX_CHECK(PKIX_List_SetImmutable(qualifiers, plContext),
                PKIX_LISTSETIMMUTABLEFAILED);

        *pQualifiers = qualifiers;

cleanup:

        PKIX_RETURN(CERTPOLICYNODE);
}





PKIX_Error *
PKIX_PolicyNode_GetExpectedPolicies(
        PKIX_PolicyNode *node,
        PKIX_List **pExpPolicies,  
        void *plContext)
{

        PKIX_ENTER(CERTPOLICYNODE, "PKIX_PolicyNode_GetExpectedPolicies");

        PKIX_NULLCHECK_TWO(node, pExpPolicies);

        PKIX_INCREF(node->expectedPolicySet);
        *pExpPolicies = node->expectedPolicySet;

cleanup:
        PKIX_RETURN(CERTPOLICYNODE);
}





PKIX_Error *
PKIX_PolicyNode_IsCritical(
        PKIX_PolicyNode *node,
        PKIX_Boolean *pCritical,
        void *plContext)
{

        PKIX_ENTER(CERTPOLICYNODE, "PKIX_PolicyNode_IsCritical");

        PKIX_NULLCHECK_TWO(node, pCritical);

        *pCritical = node->criticality;

        PKIX_RETURN(CERTPOLICYNODE);
}





PKIX_Error *
PKIX_PolicyNode_GetDepth(
        PKIX_PolicyNode *node,
        PKIX_UInt32 *pDepth,
        void *plContext)
{

        PKIX_ENTER(CERTPOLICYNODE, "PKIX_PolicyNode_GetDepth");

        PKIX_NULLCHECK_TWO(node, pDepth);

        *pDepth = node->depth;

        PKIX_RETURN(CERTPOLICYNODE);
}

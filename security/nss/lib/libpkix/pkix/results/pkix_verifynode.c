










































#include "pkix_verifynode.h"




























PKIX_Error *
pkix_VerifyNode_Create(
        PKIX_PL_Cert *cert,
        PKIX_UInt32 depth,
        PKIX_Error *error,
        PKIX_VerifyNode **pObject,
        void *plContext)
{
        PKIX_VerifyNode *node = NULL;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_Create");
        PKIX_NULLCHECK_TWO(cert, pObject);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_VERIFYNODE_TYPE,
                sizeof (PKIX_VerifyNode),
                (PKIX_PL_Object **)&node,
                plContext),
                PKIX_COULDNOTCREATEVERIFYNODEOBJECT);

        PKIX_INCREF(cert);
        node->verifyCert = cert;

        PKIX_INCREF(error);
        node->error = error;

        node->depth = depth;

        node->children = NULL;

        *pObject = node;
        node = NULL;

cleanup:

        PKIX_DECREF(node);

        PKIX_RETURN(VERIFYNODE);
}


































PKIX_Error *
pkix_VerifyNode_AddToChain(
        PKIX_VerifyNode *parentNode,
        PKIX_VerifyNode *child,
        void *plContext)
{
        PKIX_VerifyNode *successor = NULL;
        PKIX_List *listOfChildren = NULL;
        PKIX_UInt32 numChildren = 0;
        PKIX_UInt32 parentDepth = 0;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_AddToChain");
        PKIX_NULLCHECK_TWO(parentNode, child);

        parentDepth = parentNode->depth;
        listOfChildren = parentNode->children;
        if (listOfChildren == NULL) {

                if (parentDepth != (child->depth - 1)) {
                        PKIX_ERROR(PKIX_NODESMISSINGFROMCHAIN);
                }

                PKIX_CHECK(PKIX_List_Create(&listOfChildren, plContext),
                        PKIX_LISTCREATEFAILED);

                PKIX_CHECK(PKIX_List_AppendItem
                        (listOfChildren, (PKIX_PL_Object *)child, plContext),
                        PKIX_COULDNOTAPPENDCHILDTOPARENTSVERIFYNODELIST);

                parentNode->children = listOfChildren;
        } else {
                
                PKIX_CHECK(PKIX_List_GetLength
                        (listOfChildren, &numChildren, plContext),
                        PKIX_LISTGETLENGTHFAILED);

                if (numChildren != 1) {
                        PKIX_ERROR(PKIX_AMBIGUOUSPARENTAGEOFVERIFYNODE);
                }

                
                PKIX_CHECK(PKIX_List_GetItem
                        (listOfChildren,
                        0,
                        (PKIX_PL_Object **)&successor,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(pkix_VerifyNode_AddToChain
                        (successor, child, plContext),
                        PKIX_VERIFYNODEADDTOCHAINFAILED);
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)parentNode, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

cleanup:
        PKIX_DECREF(successor);

        PKIX_RETURN(VERIFYNODE);
}






















static PKIX_Error *
pkix_VerifyNode_SetDepth(PKIX_List *children,
        PKIX_UInt32 depth,
        void *plContext)
{
        PKIX_UInt32 numChildren = 0;
        PKIX_UInt32 chIx = 0;
        PKIX_VerifyNode *child = NULL;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_SetDepth");
        PKIX_NULLCHECK_ONE(children);

        PKIX_CHECK(PKIX_List_GetLength(children, &numChildren, plContext),
                PKIX_LISTGETLENGTHFAILED);

        for (chIx = 0; chIx < numChildren; chIx++) {
               PKIX_CHECK(PKIX_List_GetItem
                        (children, chIx, (PKIX_PL_Object **)&child, plContext),
                        PKIX_LISTGETITEMFAILED);

                child->depth = depth;

                if (child->children != NULL) {
                        PKIX_CHECK(pkix_VerifyNode_SetDepth
                                (child->children, depth + 1, plContext),
                                PKIX_VERIFYNODESETDEPTHFAILED);
                }

                PKIX_DECREF(child);
        }

cleanup:

        PKIX_DECREF(child);

        PKIX_RETURN(VERIFYNODE);
}































PKIX_Error *
pkix_VerifyNode_AddToTree(
        PKIX_VerifyNode *parentNode,
        PKIX_VerifyNode *child,
        void *plContext)
{
        PKIX_List *listOfChildren = NULL;
        PKIX_UInt32 parentDepth = 0;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_AddToTree");
        PKIX_NULLCHECK_TWO(parentNode, child);

        parentDepth = parentNode->depth;
        listOfChildren = parentNode->children;
        if (listOfChildren == NULL) {

                PKIX_CHECK(PKIX_List_Create(&listOfChildren, plContext),
                        PKIX_LISTCREATEFAILED);

                parentNode->children = listOfChildren;
        }

        child->depth = parentDepth + 1;

        PKIX_CHECK(PKIX_List_AppendItem
                (parentNode->children, (PKIX_PL_Object *)child, plContext),
                PKIX_COULDNOTAPPENDCHILDTOPARENTSVERIFYNODELIST);

        if (child->children != NULL) {
                PKIX_CHECK(pkix_VerifyNode_SetDepth
                        (child->children, child->depth + 1, plContext),
                        PKIX_VERIFYNODESETDEPTHFAILED);
        }


cleanup:

        PKIX_RETURN(VERIFYNODE);
}























PKIX_Error *
pkix_SingleVerifyNode_ToString(
        PKIX_VerifyNode *node,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_String *fmtString = NULL;
        PKIX_PL_String *errorString = NULL;
        PKIX_PL_String *outString = NULL;

        PKIX_PL_X500Name *issuerName = NULL;
        PKIX_PL_X500Name *subjectName = NULL;
        PKIX_PL_String *issuerString = NULL;
        PKIX_PL_String *subjectString = NULL;

        PKIX_ENTER(VERIFYNODE, "pkix_SingleVerifyNode_ToString");
        PKIX_NULLCHECK_THREE(node, pString, node->verifyCert);

        PKIX_TOSTRING(node->error, &errorString, plContext,
                PKIX_ERRORTOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_Cert_GetIssuer
                (node->verifyCert, &issuerName, plContext),
                PKIX_CERTGETISSUERFAILED);

        PKIX_TOSTRING(issuerName, &issuerString, plContext,
                PKIX_X500NAMETOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_Cert_GetSubject
                (node->verifyCert, &subjectName, plContext),
                PKIX_CERTGETSUBJECTFAILED);

        PKIX_TOSTRING(subjectName, &subjectString, plContext,
                PKIX_X500NAMETOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII,
                "CERT[Issuer:%s, Subject:%s], depth=%d, error=%s",
                0,
                &fmtString,
                plContext),
                PKIX_CANTCREATESTRING);

        PKIX_CHECK(PKIX_PL_Sprintf
                (&outString,
                plContext,
                fmtString,
                issuerString,
                subjectString,
                node->depth,
                errorString),
                PKIX_SPRINTFFAILED);

        *pString = outString;

cleanup:

        PKIX_DECREF(fmtString);
        PKIX_DECREF(errorString);
        PKIX_DECREF(issuerName);
        PKIX_DECREF(subjectName);
        PKIX_DECREF(issuerString);
        PKIX_DECREF(subjectString);
        PKIX_RETURN(VERIFYNODE);
}





























static PKIX_Error *
pkix_VerifyNode_ToString_Helper(
        PKIX_VerifyNode *rootNode,
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
        PKIX_VerifyNode *childNode = NULL;
        PKIX_UInt32 numberOfChildren = 0;
        PKIX_UInt32 childIndex = 0;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_ToString_Helper");

        PKIX_NULLCHECK_TWO(rootNode, pTreeString);

        
        PKIX_CHECK(pkix_SingleVerifyNode_ToString
                (rootNode, &thisItemString, plContext),
                PKIX_ERRORINSINGLEVERIFYNODETOSTRING);

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

                        PKIX_CHECK(pkix_VerifyNode_ToString_Helper
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

        PKIX_RETURN(VERIFYNODE);
}





static PKIX_Error *
pkix_VerifyNode_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pTreeString,
        void *plContext)
{
        PKIX_VerifyNode *rootNode = NULL;
        PKIX_PL_String *resultString = NULL;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_ToString");

        PKIX_NULLCHECK_TWO(object, pTreeString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_VERIFYNODE_TYPE, plContext),
                PKIX_OBJECTNOTVERIFYNODE);

        rootNode = (PKIX_VerifyNode *)object;

        PKIX_CHECK(pkix_VerifyNode_ToString_Helper
                (rootNode, NULL, &resultString, plContext),
                PKIX_ERRORCREATINGSUBTREESTRING);

        *pTreeString = resultString;

cleanup:

        PKIX_RETURN(VERIFYNODE);
}





static PKIX_Error *
pkix_VerifyNode_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_VerifyNode *node = NULL;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_Destroy");

        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_VERIFYNODE_TYPE, plContext),
                PKIX_OBJECTNOTVERIFYNODE);

        node = (PKIX_VerifyNode*)object;

        PKIX_DECREF(node->verifyCert);
        PKIX_DECREF(node->children);
        PKIX_DECREF(node->error);

        node->depth = 0;

cleanup:

        PKIX_RETURN(VERIFYNODE);
}
























static PKIX_Error *
pkix_SingleVerifyNode_Hashcode(
        PKIX_VerifyNode *node,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_UInt32 errorHash = 0;
        PKIX_UInt32 nodeHash = 0;

        PKIX_ENTER(VERIFYNODE, "pkix_SingleVerifyNode_Hashcode");
        PKIX_NULLCHECK_TWO(node, pHashcode);

        PKIX_HASHCODE
                (node->verifyCert,
                &nodeHash,
                plContext,
                PKIX_FAILUREHASHINGCERT);

        PKIX_CHECK(PKIX_PL_Object_Hashcode
                ((PKIX_PL_Object *)node->error,
                &errorHash,
                plContext),
                PKIX_FAILUREHASHINGERROR);

        nodeHash = 31*nodeHash + errorHash;
        *pHashcode = nodeHash;

cleanup:

        PKIX_RETURN(VERIFYNODE);
}





static PKIX_Error *
pkix_VerifyNode_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_VerifyNode *node = NULL;
        PKIX_UInt32 childrenHash = 0;
        PKIX_UInt32 nodeHash = 0;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_VERIFYNODE_TYPE, plContext),
                PKIX_OBJECTNOTVERIFYNODE);

        node = (PKIX_VerifyNode *)object;

        PKIX_CHECK(pkix_SingleVerifyNode_Hashcode
                (node, &nodeHash, plContext),
                PKIX_SINGLEVERIFYNODEHASHCODEFAILED);

        PKIX_HASHCODE
                (node->children,
                &childrenHash,
                plContext,
                PKIX_OBJECTHASHCODEFAILED);

        nodeHash = 31*nodeHash + childrenHash;

        *pHashcode = nodeHash;

cleanup:

        PKIX_RETURN(VERIFYNODE);
}



























static PKIX_Error *
pkix_SingleVerifyNode_Equals(
        PKIX_VerifyNode *firstVN,
        PKIX_VerifyNode *secondVN,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_Boolean compResult = PKIX_FALSE;

        PKIX_ENTER(VERIFYNODE, "pkix_SingleVerifyNode_Equals");
        PKIX_NULLCHECK_THREE(firstVN, secondVN, pResult);

        
        if (firstVN == secondVN) {
                compResult = PKIX_TRUE;
                goto cleanup;
        }

        



        if ((firstVN->depth) != (secondVN->depth)) {
                goto cleanup;
        }

        
        PKIX_NULLCHECK_TWO(firstVN->verifyCert, secondVN->verifyCert);

        PKIX_EQUALS
                (firstVN->verifyCert,
                secondVN->verifyCert,
                &compResult,
                plContext,
                PKIX_OBJECTEQUALSFAILED);

        if (compResult == PKIX_FALSE) {
                goto cleanup;
        }

        PKIX_EQUALS
                (firstVN->error,
                secondVN->error,
                &compResult,
                plContext,
                PKIX_OBJECTEQUALSFAILED);

cleanup:

        *pResult = compResult;

        PKIX_RETURN(VERIFYNODE);
}





static PKIX_Error *
pkix_VerifyNode_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_VerifyNode *firstVN = NULL;
        PKIX_VerifyNode *secondVN = NULL;
        PKIX_UInt32 secondType;
        PKIX_Boolean compResult = PKIX_FALSE;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType
                (firstObject, PKIX_VERIFYNODE_TYPE, plContext),
                PKIX_FIRSTOBJECTNOTVERIFYNODE);

        



        if (firstObject == secondObject){
                compResult = PKIX_TRUE;
                goto cleanup;
        }

        



        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObject, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);

        if (secondType != PKIX_VERIFYNODE_TYPE) {
                goto cleanup;
        }

        



        firstVN = (PKIX_VerifyNode *)firstObject;
        secondVN = (PKIX_VerifyNode *)secondObject;

        PKIX_CHECK(pkix_SingleVerifyNode_Equals
                (firstVN, secondVN, &compResult, plContext),
                PKIX_SINGLEVERIFYNODEEQUALSFAILED);

        if (compResult == PKIX_FALSE) {
                goto cleanup;
        }

        PKIX_EQUALS
                (firstVN->children,
                secondVN->children,
                &compResult,
                plContext,
                PKIX_OBJECTEQUALSFAILEDONCHILDREN);

cleanup:

        *pResult = compResult;

        PKIX_RETURN(VERIFYNODE);
}

































static PKIX_Error *
pkix_VerifyNode_DuplicateHelper(
        PKIX_VerifyNode *original,
        PKIX_VerifyNode *parent,
        PKIX_VerifyNode **pNewNode,
        void *plContext)
{
        PKIX_UInt32 numChildren = 0;
        PKIX_UInt32 childIndex = 0;
        PKIX_List *children = NULL; 
        PKIX_VerifyNode *copy = NULL;
        PKIX_VerifyNode *child = NULL;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_DuplicateHelper");

        PKIX_NULLCHECK_TWO
                (original, original->verifyCert);

        




        PKIX_CHECK(pkix_VerifyNode_Create
                (original->verifyCert,
                original->depth,
                original->error,
                &copy,
                plContext),
                PKIX_VERIFYNODECREATEFAILED);

        
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

                PKIX_CHECK(pkix_VerifyNode_DuplicateHelper
                        (child, copy, NULL, plContext),
                        PKIX_VERIFYNODEDUPLICATEHELPERFAILED);

                PKIX_DECREF(child);
        }

        if (pNewNode) {
                *pNewNode = copy;
                copy = NULL; 
        }

cleanup:
        PKIX_DECREF(copy);
        PKIX_DECREF(child);

        PKIX_RETURN(VERIFYNODE);
}





static PKIX_Error *
pkix_VerifyNode_Duplicate(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext)
{
        PKIX_VerifyNode *original = NULL;
        PKIX_VerifyNode *copy = NULL;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_Duplicate");

        PKIX_NULLCHECK_TWO(object, pNewObject);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_VERIFYNODE_TYPE, plContext),
                PKIX_OBJECTNOTVERIFYNODE);

        original = (PKIX_VerifyNode *)object;

        PKIX_CHECK(pkix_VerifyNode_DuplicateHelper
                (original, NULL, &copy, plContext),
                PKIX_VERIFYNODEDUPLICATEHELPERFAILED);

        *pNewObject = (PKIX_PL_Object *)copy;

cleanup:

        PKIX_RETURN(VERIFYNODE);
}















PKIX_Error *
pkix_VerifyNode_RegisterSelf(void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(VERIFYNODE, "pkix_VerifyNode_RegisterSelf");

        entry.description = "VerifyNode";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_VerifyNode);
        entry.destructor = pkix_VerifyNode_Destroy;
        entry.equalsFunction = pkix_VerifyNode_Equals;
        entry.hashcodeFunction = pkix_VerifyNode_Hashcode;
        entry.toStringFunction = pkix_VerifyNode_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_VerifyNode_Duplicate;

        systemClasses[PKIX_VERIFYNODE_TYPE] = entry;

        PKIX_RETURN(VERIFYNODE);
}























PKIX_Error *
pkix_VerifyNode_SetError(
        PKIX_VerifyNode *node,
        PKIX_Error *error,
        void *plContext)
{

        PKIX_ENTER(VERIFYNODE, "PKIX_VerifyNode_SetError");

        PKIX_NULLCHECK_TWO(node, error);

        PKIX_DECREF(node->error); 
        PKIX_INCREF(error);
        node->error = error;

cleanup:
        PKIX_RETURN(VERIFYNODE);
}






















PKIX_Error *
pkix_VerifyNode_FindError(
        PKIX_VerifyNode *node,
        PKIX_Error **error,
        void *plContext)
{
    PKIX_VerifyNode *childNode = NULL;

    PKIX_ENTER(VERIFYNODE, "PKIX_VerifyNode_FindError");

    
    PKIX_DECREF(*error);

    if (!node)
        goto cleanup;
    
    
    if (node->children) {
        PKIX_UInt32 length = 0;
        PKIX_UInt32 index = 0;

        PKIX_CHECK(
            PKIX_List_GetLength(node->children, &length,
                                plContext),
            PKIX_LISTGETLENGTHFAILED);
        for (index = 0;index < length;index++) {
            PKIX_CHECK(
                PKIX_List_GetItem(node->children, index,
                                  (PKIX_PL_Object**)&childNode, plContext),
                PKIX_LISTGETITEMFAILED);
            if (!childNode)
                continue;
            PKIX_CHECK(
                pkix_VerifyNode_FindError(childNode, error,
                                          plContext),
                PKIX_VERIFYNODEFINDERRORFAILED);
            PKIX_DECREF(childNode);
            if (*error) {
                goto cleanup;
            }
        }
    }
    
    if (node->error) {
        PKIX_INCREF(node->error);
        *error = node->error;
    }

cleanup:
    PKIX_DECREF(childNode);
    
    PKIX_RETURN(VERIFYNODE);
}











#ifndef _PKIX_POLICYNODE_H
#define _PKIX_POLICYNODE_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif



struct PKIX_PolicyNodeStruct {
    PKIX_PL_OID *validPolicy;
    PKIX_List *qualifierSet;    
    PKIX_Boolean criticality;
    PKIX_List *expectedPolicySet;       
    PKIX_PolicyNode *parent;
    PKIX_List *children;                
    PKIX_UInt32 depth;
};

PKIX_Error *
pkix_SinglePolicyNode_ToString(
        PKIX_PolicyNode *node,
        PKIX_PL_String **pString,
        void *plContext);

PKIX_Error *
pkix_PolicyNode_GetChildrenMutable(
        PKIX_PolicyNode *node,
        PKIX_List **pChildren,  
        void *plContext);

PKIX_Error *
pkix_PolicyNode_Create(
        PKIX_PL_OID *validPolicy,
        PKIX_List *qualifierSet,        
        PKIX_Boolean criticality,
        PKIX_List *expectedPolicySet,   
        PKIX_PolicyNode **pObject,
        void *plContext);

PKIX_Error *
pkix_PolicyNode_AddToParent(
        PKIX_PolicyNode *parentNode,
        PKIX_PolicyNode *child,
        void *plContext);

PKIX_Error *
pkix_PolicyNode_Prune(
        PKIX_PolicyNode *node,
        PKIX_UInt32 depth,
        PKIX_Boolean *pDelete,
        void *plContext);

PKIX_Error *
pkix_PolicyNode_RegisterSelf(
        void *plContext);

#ifdef __cplusplus
}
#endif

#endif

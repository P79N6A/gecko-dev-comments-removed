



















































CK_PKCS11_FUNCTION_INFO(C_Initialize)
#ifdef CK_NEED_ARG_LIST
(
  CK_VOID_PTR   pInitArgs  


);
#endif




CK_PKCS11_FUNCTION_INFO(C_Finalize)
#ifdef CK_NEED_ARG_LIST
(
  CK_VOID_PTR   pReserved  
);
#endif



CK_PKCS11_FUNCTION_INFO(C_GetInfo)
#ifdef CK_NEED_ARG_LIST
(
  CK_INFO_PTR   pInfo  
);
#endif



CK_PKCS11_FUNCTION_INFO(C_GetFunctionList)
#ifdef CK_NEED_ARG_LIST
(
  CK_FUNCTION_LIST_PTR_PTR ppFunctionList  

);
#endif






CK_PKCS11_FUNCTION_INFO(C_GetSlotList)
#ifdef CK_NEED_ARG_LIST
(
  CK_BBOOL       tokenPresent,  
  CK_SLOT_ID_PTR pSlotList,     
  CK_ULONG_PTR   pulCount       
);
#endif




CK_PKCS11_FUNCTION_INFO(C_GetSlotInfo)
#ifdef CK_NEED_ARG_LIST
(
  CK_SLOT_ID       slotID,  
  CK_SLOT_INFO_PTR pInfo    
);
#endif




CK_PKCS11_FUNCTION_INFO(C_GetTokenInfo)
#ifdef CK_NEED_ARG_LIST
(
  CK_SLOT_ID        slotID,  
  CK_TOKEN_INFO_PTR pInfo    
);
#endif




CK_PKCS11_FUNCTION_INFO(C_GetMechanismList)
#ifdef CK_NEED_ARG_LIST
(
  CK_SLOT_ID            slotID,          
  CK_MECHANISM_TYPE_PTR pMechanismList,  
  CK_ULONG_PTR          pulCount         
);
#endif




CK_PKCS11_FUNCTION_INFO(C_GetMechanismInfo)
#ifdef CK_NEED_ARG_LIST
(
  CK_SLOT_ID            slotID,  
  CK_MECHANISM_TYPE     type,    
  CK_MECHANISM_INFO_PTR pInfo    
);
#endif



CK_PKCS11_FUNCTION_INFO(C_InitToken)
#ifdef CK_NEED_ARG_LIST

(
  CK_SLOT_ID         slotID,    
  CK_UTF8CHAR_PTR    pPin,      
  CK_ULONG           ulPinLen,  
  CK_UTF8CHAR_PTR    pLabel     
);
#endif



CK_PKCS11_FUNCTION_INFO(C_InitPIN)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,  
  CK_UTF8CHAR_PTR   pPin,      
  CK_ULONG          ulPinLen   
);
#endif



CK_PKCS11_FUNCTION_INFO(C_SetPIN)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,  
  CK_UTF8CHAR_PTR   pOldPin,   
  CK_ULONG          ulOldLen,  
  CK_UTF8CHAR_PTR   pNewPin,   
  CK_ULONG          ulNewLen   
);
#endif







CK_PKCS11_FUNCTION_INFO(C_OpenSession)
#ifdef CK_NEED_ARG_LIST
(
  CK_SLOT_ID            slotID,        
  CK_FLAGS              flags,         
  CK_VOID_PTR           pApplication,  
  CK_NOTIFY             Notify,        
  CK_SESSION_HANDLE_PTR phSession      
);
#endif




CK_PKCS11_FUNCTION_INFO(C_CloseSession)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession  
);
#endif



CK_PKCS11_FUNCTION_INFO(C_CloseAllSessions)
#ifdef CK_NEED_ARG_LIST
(
  CK_SLOT_ID     slotID  
);
#endif



CK_PKCS11_FUNCTION_INFO(C_GetSessionInfo)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE   hSession,  
  CK_SESSION_INFO_PTR pInfo      
);
#endif




CK_PKCS11_FUNCTION_INFO(C_GetOperationState)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,             
  CK_BYTE_PTR       pOperationState,      
  CK_ULONG_PTR      pulOperationStateLen  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_SetOperationState)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,            
  CK_BYTE_PTR      pOperationState,      
  CK_ULONG         ulOperationStateLen,  
  CK_OBJECT_HANDLE hEncryptionKey,       
  CK_OBJECT_HANDLE hAuthenticationKey    
);
#endif



CK_PKCS11_FUNCTION_INFO(C_Login)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,  
  CK_USER_TYPE      userType,  
  CK_UTF8CHAR_PTR   pPin,      
  CK_ULONG          ulPinLen   
);
#endif



CK_PKCS11_FUNCTION_INFO(C_Logout)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession  
);
#endif






CK_PKCS11_FUNCTION_INFO(C_CreateObject)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,    
  CK_ATTRIBUTE_PTR  pTemplate,   
  CK_ULONG          ulCount,     
  CK_OBJECT_HANDLE_PTR phObject  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_CopyObject)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE    hSession,    
  CK_OBJECT_HANDLE     hObject,     
  CK_ATTRIBUTE_PTR     pTemplate,   
  CK_ULONG             ulCount,     
  CK_OBJECT_HANDLE_PTR phNewObject  
);
#endif



CK_PKCS11_FUNCTION_INFO(C_DestroyObject)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,  
  CK_OBJECT_HANDLE  hObject    
);
#endif



CK_PKCS11_FUNCTION_INFO(C_GetObjectSize)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,  
  CK_OBJECT_HANDLE  hObject,   
  CK_ULONG_PTR      pulSize    
);
#endif




CK_PKCS11_FUNCTION_INFO(C_GetAttributeValue)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,   
  CK_OBJECT_HANDLE  hObject,    
  CK_ATTRIBUTE_PTR  pTemplate,  
  CK_ULONG          ulCount     
);
#endif




CK_PKCS11_FUNCTION_INFO(C_SetAttributeValue)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,   
  CK_OBJECT_HANDLE  hObject,    
  CK_ATTRIBUTE_PTR  pTemplate,  
  CK_ULONG          ulCount     
);
#endif




CK_PKCS11_FUNCTION_INFO(C_FindObjectsInit)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,   
  CK_ATTRIBUTE_PTR  pTemplate,  
  CK_ULONG          ulCount     
);
#endif





CK_PKCS11_FUNCTION_INFO(C_FindObjects)
#ifdef CK_NEED_ARG_LIST
(
 CK_SESSION_HANDLE    hSession,          
 CK_OBJECT_HANDLE_PTR phObject,          
 CK_ULONG             ulMaxObjectCount,  
 CK_ULONG_PTR         pulObjectCount     
);
#endif




CK_PKCS11_FUNCTION_INFO(C_FindObjectsFinal)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession  
);
#endif






CK_PKCS11_FUNCTION_INFO(C_EncryptInit)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,    
  CK_MECHANISM_PTR  pMechanism,  
  CK_OBJECT_HANDLE  hKey         
);
#endif



CK_PKCS11_FUNCTION_INFO(C_Encrypt)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,            
  CK_BYTE_PTR       pData,               
  CK_ULONG          ulDataLen,           
  CK_BYTE_PTR       pEncryptedData,      
  CK_ULONG_PTR      pulEncryptedDataLen  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_EncryptUpdate)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,           
  CK_BYTE_PTR       pPart,              
  CK_ULONG          ulPartLen,          
  CK_BYTE_PTR       pEncryptedPart,     
  CK_ULONG_PTR      pulEncryptedPartLen 
);
#endif




CK_PKCS11_FUNCTION_INFO(C_EncryptFinal)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,                
  CK_BYTE_PTR       pLastEncryptedPart,      
  CK_ULONG_PTR      pulLastEncryptedPartLen  
);
#endif



CK_PKCS11_FUNCTION_INFO(C_DecryptInit)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,    
  CK_MECHANISM_PTR  pMechanism,  
  CK_OBJECT_HANDLE  hKey         
);
#endif



CK_PKCS11_FUNCTION_INFO(C_Decrypt)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,           
  CK_BYTE_PTR       pEncryptedData,     
  CK_ULONG          ulEncryptedDataLen, 
  CK_BYTE_PTR       pData,              
  CK_ULONG_PTR      pulDataLen          
);
#endif




CK_PKCS11_FUNCTION_INFO(C_DecryptUpdate)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,            
  CK_BYTE_PTR       pEncryptedPart,      
  CK_ULONG          ulEncryptedPartLen,  
  CK_BYTE_PTR       pPart,               
  CK_ULONG_PTR      pulPartLen           
);
#endif




CK_PKCS11_FUNCTION_INFO(C_DecryptFinal)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,       
  CK_BYTE_PTR       pLastPart,      
  CK_ULONG_PTR      pulLastPartLen  
);
#endif






CK_PKCS11_FUNCTION_INFO(C_DigestInit)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,   
  CK_MECHANISM_PTR  pMechanism  
);
#endif



CK_PKCS11_FUNCTION_INFO(C_Digest)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,     
  CK_BYTE_PTR       pData,        
  CK_ULONG          ulDataLen,    
  CK_BYTE_PTR       pDigest,      
  CK_ULONG_PTR      pulDigestLen  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_DigestUpdate)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,  
  CK_BYTE_PTR       pPart,     
  CK_ULONG          ulPartLen  
);
#endif





CK_PKCS11_FUNCTION_INFO(C_DigestKey)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,  
  CK_OBJECT_HANDLE  hKey       
);
#endif




CK_PKCS11_FUNCTION_INFO(C_DigestFinal)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,     
  CK_BYTE_PTR       pDigest,      
  CK_ULONG_PTR      pulDigestLen  
);
#endif









CK_PKCS11_FUNCTION_INFO(C_SignInit)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,    
  CK_MECHANISM_PTR  pMechanism,  
  CK_OBJECT_HANDLE  hKey         
);
#endif





CK_PKCS11_FUNCTION_INFO(C_Sign)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,        
  CK_BYTE_PTR       pData,           
  CK_ULONG          ulDataLen,       
  CK_BYTE_PTR       pSignature,      
  CK_ULONG_PTR      pulSignatureLen  
);
#endif





CK_PKCS11_FUNCTION_INFO(C_SignUpdate)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,  
  CK_BYTE_PTR       pPart,     
  CK_ULONG          ulPartLen  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_SignFinal)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,        
  CK_BYTE_PTR       pSignature,      
  CK_ULONG_PTR      pulSignatureLen  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_SignRecoverInit)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,   
  CK_MECHANISM_PTR  pMechanism, 
  CK_OBJECT_HANDLE  hKey        
);
#endif




CK_PKCS11_FUNCTION_INFO(C_SignRecover)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,        
  CK_BYTE_PTR       pData,           
  CK_ULONG          ulDataLen,       
  CK_BYTE_PTR       pSignature,      
  CK_ULONG_PTR      pulSignatureLen  
);
#endif








CK_PKCS11_FUNCTION_INFO(C_VerifyInit)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,    
  CK_MECHANISM_PTR  pMechanism,  
  CK_OBJECT_HANDLE  hKey          
);
#endif





CK_PKCS11_FUNCTION_INFO(C_Verify)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,       
  CK_BYTE_PTR       pData,          
  CK_ULONG          ulDataLen,      
  CK_BYTE_PTR       pSignature,     
  CK_ULONG          ulSignatureLen  
);
#endif





CK_PKCS11_FUNCTION_INFO(C_VerifyUpdate)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,  
  CK_BYTE_PTR       pPart,     
  CK_ULONG          ulPartLen  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_VerifyFinal)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,       
  CK_BYTE_PTR       pSignature,     
  CK_ULONG          ulSignatureLen  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_VerifyRecoverInit)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,    
  CK_MECHANISM_PTR  pMechanism,  
  CK_OBJECT_HANDLE  hKey         
);
#endif




CK_PKCS11_FUNCTION_INFO(C_VerifyRecover)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,        
  CK_BYTE_PTR       pSignature,      
  CK_ULONG          ulSignatureLen,  
  CK_BYTE_PTR       pData,           
  CK_ULONG_PTR      pulDataLen       
);
#endif







CK_PKCS11_FUNCTION_INFO(C_DigestEncryptUpdate)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,            
  CK_BYTE_PTR       pPart,               
  CK_ULONG          ulPartLen,           
  CK_BYTE_PTR       pEncryptedPart,      
  CK_ULONG_PTR      pulEncryptedPartLen  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_DecryptDigestUpdate)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,            
  CK_BYTE_PTR       pEncryptedPart,      
  CK_ULONG          ulEncryptedPartLen,  
  CK_BYTE_PTR       pPart,               
  CK_ULONG_PTR      pulPartLen           
);
#endif




CK_PKCS11_FUNCTION_INFO(C_SignEncryptUpdate)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,            
  CK_BYTE_PTR       pPart,               
  CK_ULONG          ulPartLen,           
  CK_BYTE_PTR       pEncryptedPart,      
  CK_ULONG_PTR      pulEncryptedPartLen  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_DecryptVerifyUpdate)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,            
  CK_BYTE_PTR       pEncryptedPart,      
  CK_ULONG          ulEncryptedPartLen,  
  CK_BYTE_PTR       pPart,               
  CK_ULONG_PTR      pulPartLen           
);
#endif







CK_PKCS11_FUNCTION_INFO(C_GenerateKey)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE    hSession,    
  CK_MECHANISM_PTR     pMechanism,  
  CK_ATTRIBUTE_PTR     pTemplate,   
  CK_ULONG             ulCount,     
  CK_OBJECT_HANDLE_PTR phKey        
);
#endif




CK_PKCS11_FUNCTION_INFO(C_GenerateKeyPair)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE    hSession,                    

  CK_MECHANISM_PTR     pMechanism,                  

  CK_ATTRIBUTE_PTR     pPublicKeyTemplate,          


  CK_ULONG             ulPublicKeyAttributeCount,   

  CK_ATTRIBUTE_PTR     pPrivateKeyTemplate,         


  CK_ULONG             ulPrivateKeyAttributeCount,  

  CK_OBJECT_HANDLE_PTR phPublicKey,                 


  CK_OBJECT_HANDLE_PTR phPrivateKey                 


);
#endif



CK_PKCS11_FUNCTION_INFO(C_WrapKey)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,        
  CK_MECHANISM_PTR  pMechanism,      
  CK_OBJECT_HANDLE  hWrappingKey,    
  CK_OBJECT_HANDLE  hKey,            
  CK_BYTE_PTR       pWrappedKey,     
  CK_ULONG_PTR      pulWrappedKeyLen 
);
#endif




CK_PKCS11_FUNCTION_INFO(C_UnwrapKey)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE    hSession,          
  CK_MECHANISM_PTR     pMechanism,        
  CK_OBJECT_HANDLE     hUnwrappingKey,    
  CK_BYTE_PTR          pWrappedKey,       
  CK_ULONG             ulWrappedKeyLen,   
  CK_ATTRIBUTE_PTR     pTemplate,         
  CK_ULONG             ulAttributeCount,  
  CK_OBJECT_HANDLE_PTR phKey              
);
#endif




CK_PKCS11_FUNCTION_INFO(C_DeriveKey)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE    hSession,          
  CK_MECHANISM_PTR     pMechanism,        
  CK_OBJECT_HANDLE     hBaseKey,          
  CK_ATTRIBUTE_PTR     pTemplate,         
  CK_ULONG             ulAttributeCount,  
  CK_OBJECT_HANDLE_PTR phKey              
);
#endif







CK_PKCS11_FUNCTION_INFO(C_SeedRandom)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,  
  CK_BYTE_PTR       pSeed,     
  CK_ULONG          ulSeedLen  
);
#endif



CK_PKCS11_FUNCTION_INFO(C_GenerateRandom)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession,    
  CK_BYTE_PTR       RandomData,  
  CK_ULONG          ulRandomLen  
);
#endif








CK_PKCS11_FUNCTION_INFO(C_GetFunctionStatus)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession  
);
#endif




CK_PKCS11_FUNCTION_INFO(C_CancelFunction)
#ifdef CK_NEED_ARG_LIST
(
  CK_SESSION_HANDLE hSession  
);
#endif







CK_PKCS11_FUNCTION_INFO(C_WaitForSlotEvent)
#ifdef CK_NEED_ARG_LIST
(
  CK_FLAGS flags,        
  CK_SLOT_ID_PTR pSlot,  
  CK_VOID_PTR pRserved   
);
#endif







































#ifndef nsXBLContentSink_h__
#define nsXBLContentSink_h__

#include "nsXMLContentSink.h"
#include "nsXBLDocumentInfo.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLProtoImpl.h"
#include "nsICSSParser.h"
#include "nsLayoutCID.h"




typedef enum {
  eXBL_InDocument,       
  eXBL_InBindings,       
  eXBL_InBinding,        
  eXBL_InResources,      
  eXBL_InImplementation, 
  eXBL_InHandlers,       
  eXBL_Error             
} XBLPrimaryState;





typedef enum {
  eXBL_None,
  eXBL_InHandler,
  eXBL_InMethod,
  eXBL_InProperty,
  eXBL_InField,
  eXBL_InBody,
  eXBL_InGetter,
  eXBL_InSetter,
  eXBL_InConstructor,
  eXBL_InDestructor
} XBLSecondaryState;

class nsXULPrototypeElement;
class nsXBLProtoImplMember;
class nsXBLProtoImplProperty;
class nsXBLProtoImplMethod;
class nsXBLProtoImplField;
class nsXBLPrototypeBinding;





class nsXBLContentSink : public nsXMLContentSink {
public:
  nsXBLContentSink();
  ~nsXBLContentSink();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  nsresult Init(nsIDocument* aDoc,
                nsIURI* aURL,
                nsISupports* aContainer);

  
  NS_IMETHOD HandleStartElement(const PRUnichar *aName, 
                                const PRUnichar **aAtts, 
                                PRUint32 aAttsCount, 
                                PRInt32 aIndex, 
                                PRUint32 aLineNumber);

  NS_IMETHOD HandleEndElement(const PRUnichar *aName);
  
  NS_IMETHOD HandleCDataSection(const PRUnichar *aData, 
                                PRUint32 aLength);

protected:
    
    virtual void MaybeStartLayout(PRBool aIgnorePendingSheets);

    PRBool OnOpenContainer(const PRUnichar **aAtts, 
                           PRUint32 aAttsCount, 
                           PRInt32 aNameSpaceID, 
                           nsIAtom* aTagName,
                           PRUint32 aLineNumber);
    
    nsresult CreateElement(const PRUnichar** aAtts, PRUint32 aAttsCount,
                           nsINodeInfo* aNodeInfo, PRUint32 aLineNumber,
                           nsIContent** aResult, PRBool* aAppendContent);
    
    nsresult AddAttributes(const PRUnichar** aAtts, 
                           nsIContent* aContent);

#ifdef MOZ_XUL    
    nsresult AddAttributesToXULPrototype(const PRUnichar **aAtts, 
                                         PRUint32 aAttsCount, 
                                         nsXULPrototypeElement* aElement);
#endif

    
    nsresult ConstructBinding();
    void ConstructHandler(const PRUnichar **aAtts, PRUint32 aLineNumber);
    void ConstructResource(const PRUnichar **aAtts, nsIAtom* aResourceType);
    void ConstructImplementation(const PRUnichar **aAtts);
    void ConstructProperty(const PRUnichar **aAtts);
    void ConstructMethod(const PRUnichar **aAtts);
    void ConstructParameter(const PRUnichar **aAtts);
    void ConstructField(const PRUnichar **aAtts, PRUint32 aLineNumber);
  

  
  nsresult FlushText(PRBool aCreateTextNode=PR_TRUE,
                     PRBool* aDidFlush=nsnull);

  
  NS_IMETHOD ReportError(const PRUnichar* aErrorText,
                         const PRUnichar* aSourceText,
                         nsIScriptError *aError,
                         PRBool *_retval);

protected:
  nsresult ReportUnexpectedElement(nsIAtom* aElementName, PRUint32 aLineNumber);

  void AddMember(nsXBLProtoImplMember* aMember);
  
  XBLPrimaryState mState;
  XBLSecondaryState mSecondaryState;
  nsIXBLDocumentInfo* mDocInfo;
  PRPackedBool mIsChromeOrResource; 
  PRPackedBool mFoundFirstBinding;

  nsXBLPrototypeBinding* mBinding;
  nsXBLPrototypeHandler* mHandler; 
  nsXBLProtoImpl* mImplementation;
  nsXBLProtoImplMember* mImplMember;
  nsXBLProtoImplProperty* mProperty;
  nsXBLProtoImplMethod* mMethod;
  nsXBLProtoImplField* mField;
};

nsresult
NS_NewXBLContentSink(nsIXMLContentSink** aResult,
                     nsIDocument* aDoc,
                     nsIURI* aURL,
                     nsISupports* aContainer);
#endif 

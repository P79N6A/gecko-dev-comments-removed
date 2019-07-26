




#ifndef nsXBLContentSink_h__
#define nsXBLContentSink_h__

#include "nsXMLContentSink.h"
#include "nsXBLDocumentInfo.h"
#include "nsXBLPrototypeHandler.h"
#include "nsXBLProtoImpl.h"
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
                                uint32_t aAttsCount, 
                                int32_t aIndex, 
                                uint32_t aLineNumber);

  NS_IMETHOD HandleEndElement(const PRUnichar *aName);
  
  NS_IMETHOD HandleCDataSection(const PRUnichar *aData, 
                                uint32_t aLength);

protected:
    
    virtual void MaybeStartLayout(bool aIgnorePendingSheets);

    bool OnOpenContainer(const PRUnichar **aAtts, 
                           uint32_t aAttsCount, 
                           int32_t aNameSpaceID, 
                           nsIAtom* aTagName,
                           uint32_t aLineNumber);

    bool NotifyForDocElement() { return false; }

    nsresult CreateElement(const PRUnichar** aAtts, uint32_t aAttsCount,
                           nsINodeInfo* aNodeInfo, uint32_t aLineNumber,
                           nsIContent** aResult, bool* aAppendContent,
                           mozilla::dom::FromParser aFromParser);
    
    nsresult AddAttributes(const PRUnichar** aAtts, 
                           nsIContent* aContent);

#ifdef MOZ_XUL    
    nsresult AddAttributesToXULPrototype(const PRUnichar **aAtts, 
                                         uint32_t aAttsCount, 
                                         nsXULPrototypeElement* aElement);
#endif

    
    nsresult ConstructBinding(uint32_t aLineNumber);
    void ConstructHandler(const PRUnichar **aAtts, uint32_t aLineNumber);
    void ConstructResource(const PRUnichar **aAtts, nsIAtom* aResourceType);
    void ConstructImplementation(const PRUnichar **aAtts);
    void ConstructProperty(const PRUnichar **aAtts);
    void ConstructMethod(const PRUnichar **aAtts);
    void ConstructParameter(const PRUnichar **aAtts);
    void ConstructField(const PRUnichar **aAtts, uint32_t aLineNumber);
  

  
  nsresult FlushText(bool aReleaseTextNode = true);

  
  NS_IMETHOD ReportError(const PRUnichar* aErrorText,
                         const PRUnichar* aSourceText,
                         nsIScriptError *aError,
                         bool *_retval);

protected:
  nsresult ReportUnexpectedElement(nsIAtom* aElementName, uint32_t aLineNumber);

  void AddMember(nsXBLProtoImplMember* aMember);
  void AddField(nsXBLProtoImplField* aField);
  
  XBLPrimaryState mState;
  XBLSecondaryState mSecondaryState;
  nsXBLDocumentInfo* mDocInfo;
  bool mIsChromeOrResource; 
  bool mFoundFirstBinding;

  nsXBLPrototypeBinding* mBinding;
  nsXBLPrototypeHandler* mHandler; 
  nsXBLProtoImpl* mImplementation;
  nsXBLProtoImplMember* mImplMember;
  nsXBLProtoImplField* mImplField;
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

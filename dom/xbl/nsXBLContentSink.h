




#ifndef nsXBLContentSink_h__
#define nsXBLContentSink_h__

#include "mozilla/Attributes.h"
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

  
  NS_IMETHOD HandleStartElement(const char16_t *aName, 
                                const char16_t **aAtts, 
                                uint32_t aAttsCount, 
                                uint32_t aLineNumber) MOZ_OVERRIDE;

  NS_IMETHOD HandleEndElement(const char16_t *aName) MOZ_OVERRIDE;
  
  NS_IMETHOD HandleCDataSection(const char16_t *aData, 
                                uint32_t aLength) MOZ_OVERRIDE;

protected:
    
    virtual void MaybeStartLayout(bool aIgnorePendingSheets) MOZ_OVERRIDE;

    bool OnOpenContainer(const char16_t **aAtts, 
                           uint32_t aAttsCount, 
                           int32_t aNameSpaceID, 
                           nsIAtom* aTagName,
                           uint32_t aLineNumber) MOZ_OVERRIDE;

    bool NotifyForDocElement() MOZ_OVERRIDE { return false; }

    nsresult CreateElement(const char16_t** aAtts, uint32_t aAttsCount,
                           nsINodeInfo* aNodeInfo, uint32_t aLineNumber,
                           nsIContent** aResult, bool* aAppendContent,
                           mozilla::dom::FromParser aFromParser) MOZ_OVERRIDE;
    
    nsresult AddAttributes(const char16_t** aAtts, 
                           nsIContent* aContent) MOZ_OVERRIDE;

#ifdef MOZ_XUL    
    nsresult AddAttributesToXULPrototype(const char16_t **aAtts, 
                                         uint32_t aAttsCount, 
                                         nsXULPrototypeElement* aElement);
#endif

    
    nsresult ConstructBinding(uint32_t aLineNumber);
    void ConstructHandler(const char16_t **aAtts, uint32_t aLineNumber);
    void ConstructResource(const char16_t **aAtts, nsIAtom* aResourceType);
    void ConstructImplementation(const char16_t **aAtts);
    void ConstructProperty(const char16_t **aAtts, uint32_t aLineNumber);
    void ConstructMethod(const char16_t **aAtts);
    void ConstructParameter(const char16_t **aAtts);
    void ConstructField(const char16_t **aAtts, uint32_t aLineNumber);
  

  
  nsresult FlushText(bool aReleaseTextNode = true) MOZ_OVERRIDE;

  
  NS_IMETHOD ReportError(const char16_t* aErrorText,
                         const char16_t* aSourceText,
                         nsIScriptError *aError,
                         bool *_retval) MOZ_OVERRIDE;

protected:
  nsresult ReportUnexpectedElement(nsIAtom* aElementName, uint32_t aLineNumber);

  void AddMember(nsXBLProtoImplMember* aMember);
  void AddField(nsXBLProtoImplField* aField);
  
  XBLPrimaryState mState;
  XBLSecondaryState mSecondaryState;
  nsXBLDocumentInfo* mDocInfo;
  bool mIsChromeOrResource; 
  bool mFoundFirstBinding;

  nsString mCurrentBindingID;

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

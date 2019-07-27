





#include "nsReferencedElement.h"
#include "nsContentUtils.h"
#include "nsIURI.h"
#include "nsBindingManager.h"
#include "nsEscape.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsCycleCollectionParticipant.h"

void
nsReferencedElement::Reset(nsIContent* aFromContent, nsIURI* aURI,
                           bool aWatch, bool aReferenceImage)
{
  NS_ABORT_IF_FALSE(aFromContent, "Reset() expects non-null content pointer");

  Unlink();

  if (!aURI)
    return;

  nsAutoCString refPart;
  aURI->GetRef(refPart);
  
  
  NS_UnescapeURL(refPart);

  nsAutoCString charset;
  aURI->GetOriginCharset(charset);
  nsAutoString ref;
  nsresult rv = nsContentUtils::ConvertStringFromEncoding(charset,
                                                          refPart,
                                                          ref);
  if (NS_FAILED(rv)) {
    
    
    
    CopyUTF8toUTF16(refPart, ref);
  }
  if (ref.IsEmpty())
    return;

  
  nsIDocument *doc = aFromContent->GetComposedDoc();
  if (!doc)
    return;

  nsIContent* bindingParent = aFromContent->GetBindingParent();
  if (bindingParent) {
    nsXBLBinding* binding = bindingParent->GetXBLBinding();
    if (binding) {
      bool isEqualExceptRef;
      rv = aURI->EqualsExceptRef(binding->PrototypeBinding()->DocURI(),
                                 &isEqualExceptRef);
      if (NS_SUCCEEDED(rv) && isEqualExceptRef) {
        
        
        
        
        
        
        
        
        
        nsINodeList* anonymousChildren =
          doc->BindingManager()->GetAnonymousNodesFor(bindingParent);

        if (anonymousChildren) {
          uint32_t length;
          anonymousChildren->GetLength(&length);
          for (uint32_t i = 0; i < length && !mElement; ++i) {
            mElement =
              nsContentUtils::MatchElementId(anonymousChildren->Item(i), ref);
          }
        }

        
        return;
      }
    }
  }

  bool isEqualExceptRef;
  rv = aURI->EqualsExceptRef(doc->GetDocumentURI(), &isEqualExceptRef);
  if (NS_FAILED(rv) || !isEqualExceptRef) {
    nsRefPtr<nsIDocument::ExternalResourceLoad> load;
    doc = doc->RequestExternalResource(aURI, aFromContent,
                                       getter_AddRefs(load));
    if (!doc) {
      if (!load || !aWatch) {
        
        return;
      }

      DocumentLoadNotification* observer =
        new DocumentLoadNotification(this, ref);
      mPendingNotification = observer;
      if (observer) {
        load->AddObserver(observer);
      }
      
    }
  }

  if (aWatch) {
    nsCOMPtr<nsIAtom> atom = do_GetAtom(ref);
    if (!atom)
      return;
    atom.swap(mWatchID);
  }

  mReferencingImage = aReferenceImage;

  HaveNewDocument(doc, aWatch, ref);
}

void
nsReferencedElement::ResetWithID(nsIContent* aFromContent, const nsString& aID,
                                 bool aWatch)
{
  nsIDocument *doc = aFromContent->GetComposedDoc();
  if (!doc)
    return;

  

  if (aWatch) {
    nsCOMPtr<nsIAtom> atom = do_GetAtom(aID);
    if (!atom)
      return;
    atom.swap(mWatchID);
  }

  mReferencingImage = false;

  HaveNewDocument(doc, aWatch, aID);
}

void
nsReferencedElement::HaveNewDocument(nsIDocument* aDocument, bool aWatch,
                                     const nsString& aRef)
{
  if (aWatch) {
    mWatchDocument = aDocument;
    if (mWatchDocument) {
      mElement = mWatchDocument->AddIDTargetObserver(mWatchID, Observe, this,
                                                     mReferencingImage);
    }
    return;
  }
  
  if (!aDocument) {
    return;
  }

  Element *e = mReferencingImage ? aDocument->LookupImageElement(aRef) :
                                   aDocument->GetElementById(aRef);
  if (e) {
    mElement = e;
  }
}

void
nsReferencedElement::Traverse(nsCycleCollectionTraversalCallback* aCB)
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCB, "mWatchDocument");
  aCB->NoteXPCOMChild(mWatchDocument);
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCB, "mContent");
  aCB->NoteXPCOMChild(mElement);
}

void
nsReferencedElement::Unlink()
{
  if (mWatchDocument && mWatchID) {
    mWatchDocument->RemoveIDTargetObserver(mWatchID, Observe, this,
                                           mReferencingImage);
  }
  if (mPendingNotification) {
    mPendingNotification->Clear();
    mPendingNotification = nullptr;
  }
  mWatchDocument = nullptr;
  mWatchID = nullptr;
  mElement = nullptr;
  mReferencingImage = false;
}

bool
nsReferencedElement::Observe(Element* aOldElement,
                             Element* aNewElement, void* aData)
{
  nsReferencedElement* p = static_cast<nsReferencedElement*>(aData);
  if (p->mPendingNotification) {
    p->mPendingNotification->SetTo(aNewElement);
  } else {
    NS_ASSERTION(aOldElement == p->mElement, "Failed to track content!");
    ChangeNotification* watcher =
      new ChangeNotification(p, aOldElement, aNewElement);
    p->mPendingNotification = watcher;
    nsContentUtils::AddScriptRunner(watcher);
  }
  bool keepTracking = p->IsPersistent();
  if (!keepTracking) {
    p->mWatchDocument = nullptr;
    p->mWatchID = nullptr;
  }
  return keepTracking;
}

NS_IMPL_ISUPPORTS_INHERITED0(nsReferencedElement::ChangeNotification,
                             nsRunnable)

NS_IMPL_ISUPPORTS(nsReferencedElement::DocumentLoadNotification,
                  nsIObserver)

NS_IMETHODIMP
nsReferencedElement::DocumentLoadNotification::Observe(nsISupports* aSubject,
                                                       const char* aTopic,
                                                       const char16_t* aData)
{
  NS_ASSERTION(PL_strcmp(aTopic, "external-resource-document-created") == 0,
               "Unexpected topic");
  if (mTarget) {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(aSubject);
    mTarget->mPendingNotification = nullptr;
    NS_ASSERTION(!mTarget->mElement, "Why do we have content here?");
    
    
    mTarget->HaveNewDocument(doc, mTarget->IsPersistent(), mRef);
    mTarget->ElementChanged(nullptr, mTarget->mElement);
  }
  return NS_OK;
}

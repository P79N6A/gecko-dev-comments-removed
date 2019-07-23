





































#include "nsReferencedElement.h"
#include "nsContentUtils.h"
#include "nsIURI.h"
#include "nsBindingManager.h"
#include "nsIURL.h"
#include "nsEscape.h"
#include "nsXBLPrototypeBinding.h"
#include "nsIDOMNode.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsCycleCollectionParticipant.h"

static PRBool EqualExceptRef(nsIURL* aURL1, nsIURL* aURL2)
{
  nsCOMPtr<nsIURI> u1;
  nsCOMPtr<nsIURI> u2;

  nsresult rv = aURL1->Clone(getter_AddRefs(u1));
  if (NS_SUCCEEDED(rv)) {
    rv = aURL2->Clone(getter_AddRefs(u2));
  }
  if (NS_FAILED(rv))
    return PR_FALSE;

  nsCOMPtr<nsIURL> url1 = do_QueryInterface(u1);
  nsCOMPtr<nsIURL> url2 = do_QueryInterface(u2);
  if (!url1 || !url2) {
    NS_WARNING("Cloning a URL produced a non-URL");
    return PR_FALSE;
  }
  url1->SetRef(EmptyCString());
  url2->SetRef(EmptyCString());

  PRBool equal;
  rv = url1->Equals(url2, &equal);
  return NS_SUCCEEDED(rv) && equal;
}

void
nsReferencedElement::Reset(nsIContent* aFromContent, nsIURI* aURI, PRBool aWatch)
{
  Unlink();

  nsCOMPtr<nsIURL> url = do_QueryInterface(aURI);
  if (!url)
    return;

  nsCAutoString refPart;
  url->GetRef(refPart);
  
  
  NS_UnescapeURL(refPart);

  nsCAutoString charset;
  url->GetOriginCharset(charset);
  nsAutoString ref;
  nsresult rv = nsContentUtils::ConvertStringFromCharset(charset, refPart, ref);
  if (NS_FAILED(rv)) {
    CopyUTF8toUTF16(refPart, ref);
  }
  if (ref.IsEmpty())
    return;

  
  nsIDocument *doc = aFromContent->GetCurrentDoc();
  if (!doc)
    return;

  
  
  
  nsCOMPtr<nsIURL> documentURL = do_QueryInterface(doc->GetDocumentURI());
  nsIContent* bindingParent = aFromContent->GetBindingParent();
  PRBool isXBL = PR_FALSE;
  if (bindingParent) {
    nsXBLBinding* binding = doc->BindingManager()->GetBinding(bindingParent);
    if (binding) {
      
      
      
      
      
      
      
      documentURL = do_QueryInterface(binding->PrototypeBinding()->DocURI());
      isXBL = PR_TRUE;
    }
  }
  if (!documentURL)
    return;

  if (!EqualExceptRef(url, documentURL)) {
    
    
    
    isXBL = PR_FALSE;
    nsRefPtr<nsIDocument::ExternalResourceLoad> load;
    doc = doc->RequestExternalResource(url, aFromContent, getter_AddRefs(load));
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

  
  if (isXBL) {
    nsCOMPtr<nsIDOMNodeList> anonymousChildren;
    doc->BindingManager()->
      GetAnonymousNodesFor(bindingParent, getter_AddRefs(anonymousChildren));

    if (anonymousChildren) {
      PRUint32 length;
      anonymousChildren->GetLength(&length);
      for (PRUint32 i = 0; i < length && !mContent; ++i) {
        nsCOMPtr<nsIDOMNode> node;
        anonymousChildren->Item(i, getter_AddRefs(node));
        nsCOMPtr<nsIContent> c = do_QueryInterface(node);
        if (c) {
          mContent = nsContentUtils::MatchElementId(c, ref);
        }
      }
    }

    
    return;
  }

  if (aWatch) {
    nsCOMPtr<nsIAtom> atom = do_GetAtom(ref);
    if (!atom)
      return;
    atom.swap(mWatchID);
  }

  HaveNewDocument(doc, aWatch, ref);
}

void
nsReferencedElement::HaveNewDocument(nsIDocument* aDocument, PRBool aWatch,
                                     const nsString& aRef)
{
  if (aWatch) {
    mWatchDocument = aDocument;
    if (mWatchDocument) {
      mContent = mWatchDocument->AddIDTargetObserver(mWatchID, Observe, this);
    }
    return;
  }
  
  if (!aDocument) {
    return;
  }
  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(aDocument);
  NS_ASSERTION(domDoc, "Content doesn't reference a dom Document");

  nsCOMPtr<nsIDOMElement> element;
  domDoc->GetElementById(aRef, getter_AddRefs(element));
  if (element) {
    mContent = do_QueryInterface(element);
  }
}

void
nsReferencedElement::Traverse(nsCycleCollectionTraversalCallback* aCB)
{
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCB, "mWatchDocument");
  aCB->NoteXPCOMChild(mWatchDocument);
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCB, "mContent");
  aCB->NoteXPCOMChild(mContent);
}

void
nsReferencedElement::Unlink()
{
  if (mWatchDocument && mWatchID) {
    mWatchDocument->RemoveIDTargetObserver(mWatchID, Observe, this);
  }
  if (mPendingNotification) {
    mPendingNotification->Clear();
    mPendingNotification = nsnull;
  }
  mWatchDocument = nsnull;
  mWatchID = nsnull;
  mContent = nsnull;
}

PRBool
nsReferencedElement::Observe(nsIContent* aOldContent,
                             nsIContent* aNewContent, void* aData)
{
  nsReferencedElement* p = static_cast<nsReferencedElement*>(aData);
  if (p->mPendingNotification) {
    p->mPendingNotification->SetTo(aNewContent);
  } else {
    NS_ASSERTION(aOldContent == p->mContent, "Failed to track content!");
    ChangeNotification* watcher =
      new ChangeNotification(p, aOldContent, aNewContent);
    p->mPendingNotification = watcher;
    nsContentUtils::AddScriptRunner(watcher);
  }
  PRBool keepTracking = p->IsPersistent();
  if (!keepTracking) {
    p->mWatchDocument = nsnull;
    p->mWatchID = nsnull;
  }
  return keepTracking;
}

NS_IMPL_ISUPPORTS_INHERITED0(nsReferencedElement::ChangeNotification,
                             nsRunnable)

NS_IMPL_ISUPPORTS1(nsReferencedElement::DocumentLoadNotification,
                   nsIObserver)

NS_IMETHODIMP
nsReferencedElement::DocumentLoadNotification::Observe(nsISupports* aSubject,
                                                       const char* aTopic,
                                                       const PRUnichar* aData)
{
  NS_ASSERTION(PL_strcmp(aTopic, "external-resource-document-created") == 0,
               "Unexpected topic");
  if (mTarget) {
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(aSubject);
    mTarget->mPendingNotification = nsnull;
    NS_ASSERTION(!mTarget->mContent, "Why do we have content here?");
    
    
    mTarget->HaveNewDocument(doc, mTarget->IsPersistent(), mRef);
    mTarget->ContentChanged(nsnull, mTarget->mContent);
  }
  return NS_OK;
}

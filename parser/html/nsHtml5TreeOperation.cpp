







































#include "nsHtml5TreeOperation.h"
#include "nsContentUtils.h"
#include "nsNodeUtils.h"
#include "nsAttrName.h"
#include "nsHtml5TreeBuilder.h"
#include "nsIDOMMutationEvent.h"
#include "mozAutoDocUpdate.h"
#include "nsBindingManager.h"
#include "nsXBLBinding.h"
#include "nsHtml5DocumentMode.h"

nsHtml5TreeOperation::nsHtml5TreeOperation()
 : mOpCode(eTreeOpAppend)
{
  MOZ_COUNT_CTOR(nsHtml5TreeOperation);
}

nsHtml5TreeOperation::~nsHtml5TreeOperation()
{
  MOZ_COUNT_DTOR(nsHtml5TreeOperation);
}

nsresult
nsHtml5TreeOperation::Perform(nsHtml5TreeOpExecutor* aBuilder)
{
  nsresult rv = NS_OK;
  switch(mOpCode) {
    case eTreeOpAppend: {
      aBuilder->PostPendingAppendNotification(mParent, mNode);
      rv = mParent->AppendChildTo(mNode, PR_FALSE);
      return rv;
    }
    case eTreeOpDetach: {
      aBuilder->FlushPendingAppendNotifications();
      nsIContent* parent = mNode->GetParent();
      if (parent) {
        PRUint32 pos = parent->IndexOf(mNode);
        NS_ASSERTION((pos >= 0), "Element not found as child of its parent");
        rv = parent->RemoveChildAt(pos, PR_TRUE, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      return rv;
    }
    case eTreeOpAppendChildrenToNewParent: {
      aBuilder->FlushPendingAppendNotifications();
      PRUint32 childCount = mParent->GetChildCount();
      PRBool didAppend = PR_FALSE;
      while (mNode->GetChildCount()) {
        nsCOMPtr<nsIContent> child = mNode->GetChildAt(0);
        rv = mNode->RemoveChildAt(0, PR_TRUE, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mParent->AppendChildTo(child, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
        didAppend = PR_TRUE;
      }
      if (didAppend) {
        nsNodeUtils::ContentAppended(mParent, childCount);
      }
      return rv;
    }
    case eTreeOpFosterParent: {
      nsIContent* parent = mTable->GetParent();
      if (parent && parent->IsNodeOfType(nsINode::eELEMENT)) {
        aBuilder->FlushPendingAppendNotifications();
        PRUint32 pos = parent->IndexOf(mTable);
        rv = parent->InsertChildAt(mNode, pos, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
        nsNodeUtils::ContentInserted(parent, mNode, pos);
      } else {
        aBuilder->PostPendingAppendNotification(mParent, mNode);
        rv = mParent->AppendChildTo(mNode, PR_FALSE);
      }
      return rv;
    }
    case eTreeOpAppendToDocument: {
      aBuilder->FlushPendingAppendNotifications();
      nsIDocument* doc = aBuilder->GetDocument();
      PRUint32 childCount = doc->GetChildCount();
      rv = doc->AppendChildTo(mNode, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
      nsNodeUtils::ContentInserted(doc, mNode, childCount);
      return rv;
    }
    case eTreeOpAddAttributes: {
      
      nsIDocument* document = mParent->GetCurrentDoc();
      
      PRUint32 len = mNode->GetAttrCount();
      for (PRUint32 i = 0; i < len; ++i) {
        const nsAttrName* attrName = mNode->GetAttrNameAt(i);
        nsIAtom* localName = attrName->LocalName();
        PRInt32 nsuri = attrName->NamespaceID();
        if (!mParent->HasAttr(nsuri, localName)) {
          nsAutoString value;
          mNode->GetAttr(nsuri, localName, value);
          
          
          
          PRUint32 stateMask = PRUint32(mParent->IntrinsicState());
          nsNodeUtils::AttributeWillChange(mParent, 
                                           nsuri,
                                           localName,
                                           static_cast<PRUint8>(nsIDOMMutationEvent::ADDITION));
          
          mParent->SetAttr(nsuri, localName, attrName->GetPrefix(), value, PR_FALSE);
          
          if (document || mParent->HasFlag(NODE_FORCE_XBL_BINDINGS)) {
            nsIDocument* ownerDoc = mParent->GetOwnerDoc();
            if (ownerDoc) {
              nsRefPtr<nsXBLBinding> binding =
                ownerDoc->BindingManager()->GetBinding(mParent);
              if (binding) {
                binding->AttributeChanged(localName, nsuri, PR_FALSE, PR_FALSE);
              }
            }
          }
          
          stateMask = stateMask ^ PRUint32(mParent->IntrinsicState());
          if (stateMask && document) {
            MOZ_AUTO_DOC_UPDATE(document, UPDATE_CONTENT_STATE, PR_TRUE);
            document->ContentStatesChanged(mParent, nsnull, stateMask);
          }
          nsNodeUtils::AttributeChanged(mParent, 
                                        nsuri, 
                                        localName, 
                                        static_cast<PRUint8>(nsIDOMMutationEvent::ADDITION),
                                        stateMask);
        }
      }
      return rv;
    }
    case eTreeOpDoneAddingChildren: {
      mNode->DoneAddingChildren(PR_FALSE);
      return rv;
    }
    case eTreeOpDoneCreatingElement: {
      mNode->DoneCreatingElement();
      return rv;
    }
    case eTreeOpUpdateStyleSheet: {
      aBuilder->UpdateStyleSheet(mNode);
      return rv;
    }
    case eTreeOpProcessBase: {
      rv = aBuilder->ProcessBASETag(mNode);
      return rv;
    }
    case eTreeOpProcessMeta: {
      rv = aBuilder->ProcessMETATag(mNode);
      return rv;
    }
    case eTreeOpProcessOfflineManifest: {
      aBuilder->ProcessOfflineManifest(mNode);
      return rv;
    }
    case eTreeOpStartLayout: {
      aBuilder->StartLayout(); 
      return rv;
    }
    case eTreeOpDocumentMode: {
      aBuilder->DocumentMode(mMode);
      return rv;
    }
    default: {
      NS_NOTREACHED("Bogus tree op");
    }
  }
  return rv; 
}

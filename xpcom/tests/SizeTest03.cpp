

#include "nsIDOMNode.h"
#include "nsCOMPtr.h"
#include "nsString.h"

#ifdef __MWERKS__
	#pragma exceptions off
#endif

NS_DEF_PTR(nsIDOMNode);

	














void 
Test03_raw( nsIDOMNode* aDOMNode, nsString* aResult )
		
	{






		nsIDOMNode* parent = 0;
		nsresult status = aDOMNode->GetParentNode(&parent);
		
		if ( NS_SUCCEEDED(status) )
			{
				parent->GetNodeName(*aResult);
			}

		NS_IF_RELEASE(parent);


	}


void 
Test03_raw_optimized( nsIDOMNode* aDOMNode, nsString* aResult )
		
	{



		nsIDOMNode* parent;
		nsresult status = aDOMNode->GetParentNode(&parent);
		
		if ( NS_SUCCEEDED(status) )
			{
				parent->GetNodeName(*aResult);
				NS_RELEASE(parent);
			}


	}


void 
Test03_nsCOMPtr( nsIDOMNode* aDOMNode, nsString* aResult )
		
	{



		nsCOMPtr<nsIDOMNode> parent;
		nsresult status = aDOMNode->GetParentNode( getter_AddRefs(parent) );
		if ( parent )
			parent->GetNodeName(*aResult);


	}

void 
Test03_nsCOMPtr_optimized( nsIDOMNode* aDOMNode, nsString* aResult )
		
	{



		nsIDOMNode* temp;
		nsresult status = aDOMNode->GetParentNode(&temp);
		nsCOMPtr<nsIDOMNode> parent( dont_AddRef(temp) );
		if ( parent )
			parent->GetNodeName(*aResult);


	}

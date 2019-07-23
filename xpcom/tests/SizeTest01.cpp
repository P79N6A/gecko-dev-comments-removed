

#include "nsIDOMNode.h"
#include "nsCOMPtr.h"
#include "nsString.h"

NS_DEF_PTR(nsIDOMNode);

	
































void
Test01_raw( nsIDOMNode* aDOMNode, nsString* aResult )
		
	{
			





		nsIDOMNode* node = aDOMNode;
		NS_IF_ADDREF(node);

		if ( node )
			node->GetNodeName(*aResult);

		NS_IF_RELEASE(node);
	}

void
Test01_raw_optimized( nsIDOMNode* aDOMNode, nsString* aResult )
		
	{
			





			










		nsIDOMNode* node = aDOMNode;
		NS_ADDREF(node);
		node->GetNodeName(*aResult);
		NS_RELEASE(node);
	}

void
Test01_nsCOMPtr( nsIDOMNode* aDOMNode, nsString* aResult )
		
	{
		nsCOMPtr<nsIDOMNode> node = aDOMNode;

		if ( node )
			node->GetNodeName(*aResult);
	}

void
Test01_nsCOMPtr_optimized( nsIDOMNode* aDOMNode, nsString* aResult )
		
	{



		nsCOMPtr<nsIDOMNode> node = aDOMNode;
		node->GetNodeName(*aResult);
	}



#include "nsIDOMNode.h"
#include "nsCOMPtr.h"
#include "nsString.h"

#ifdef __MWERKS__
	#pragma exceptions off
#endif

NS_DEF_PTR(nsIDOMNode);

	



























void 
Test02_Raw00( nsISupports* aDOMNode, nsString* aResult )
		
	{






		nsIDOMNode* node = 0;
		nsresult status = aDOMNode->QueryInterface(NS_GET_IID(nsIDOMNode), (void**)&node);
		if ( NS_SUCCEEDED(status) )
			{
				node->GetNodeName(*aResult);
			}

		NS_IF_RELEASE(node);


	}

void 
Test02_Raw01( nsISupports* aDOMNode, nsString* aResult )
		
	{



		nsIDOMNode* node;
                nsresult status = aDOMNode->QueryInterface(NS_GET_IID(nsIDOMNode), (void**)&node);
		if ( NS_SUCCEEDED(status) )
			{
				node->GetNodeName(*aResult);
				NS_RELEASE(node);
			}


	}

void 
Test02_nsCOMPtr( nsISupports* aDOMNode, nsString* aResult )
		
	{
		nsresult status;
		nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aDOMNode, &status);
		
		if ( node )
			node->GetNodeName(*aResult);


	}


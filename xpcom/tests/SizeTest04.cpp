

#include "nsIDOMNode.h"
#include "nsCOMPtr.h"

#ifdef __MWERKS__
	#pragma exceptions off
#endif

NS_DEF_PTR(nsIDOMNode);

	









class Test04_Raw
	{
		public:
			Test04_Raw();
		 ~Test04_Raw();

			void  SetNode( nsIDOMNode* newNode );

		private:
			nsIDOMNode* mNode;
	};

Test04_Raw::Test04_Raw()
		: mNode(0)
	{
		
	}

Test04_Raw::~Test04_Raw()
	{
		NS_IF_RELEASE(mNode);
	}

void 
Test04_Raw::SetNode( nsIDOMNode* newNode )
		
	{
		NS_IF_ADDREF(newNode);
		NS_IF_RELEASE(mNode);
		mNode = newNode;


	}



class Test04_nsCOMPtr
	{
		public:
			void  SetNode( nsIDOMNode* newNode );

		private:
			nsCOMPtr<nsIDOMNode> mNode;
	};

void 
Test04_nsCOMPtr::SetNode( nsIDOMNode* newNode )
		
	{
		mNode = newNode;
	}

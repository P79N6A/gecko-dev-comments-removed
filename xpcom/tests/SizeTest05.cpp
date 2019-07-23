

#include "nsIDOMNode.h"
#include "nsCOMPtr.h"

#ifdef __MWERKS__
	#pragma exceptions off
#endif

NS_DEF_PTR(nsIDOMNode);

	







class Test05_Raw
	{
		public:
                        Test05_Raw();
                 ~Test05_Raw();

			void  GetNode( nsIDOMNode** aNode );

		private:
			nsIDOMNode* mNode;
	};

Test05_Raw::Test05_Raw()
		: mNode(0)
	{
		
	}

Test05_Raw::~Test05_Raw()
	{
		NS_IF_RELEASE(mNode);
	}

void 
Test05_Raw::GetNode( nsIDOMNode** aNode )
		
	{



		*aNode = mNode;
		NS_IF_ADDREF(*aNode);


	}



class Test05_nsCOMPtr
	{
		public:
			void  GetNode( nsIDOMNode** aNode );

		private:
			nsCOMPtr<nsIDOMNode> mNode;
	};

void 
Test05_nsCOMPtr::GetNode( nsIDOMNode** aNode )
		
	{



		*aNode = mNode;
		NS_IF_ADDREF(*aNode);


	}

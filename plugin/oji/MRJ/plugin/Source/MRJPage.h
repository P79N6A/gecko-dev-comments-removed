











































#ifndef __TYPES__
#include <Types.h>
#endif

#ifndef CALL_NOT_IN_CARBON
	#define CALL_NOT_IN_CARBON 1
#endif

#include "JManager.h"


typedef struct OpaqueJMAppletPageRef* 	JMAppletPageRef;

class MRJSession;

struct MRJPageAttributes {
	UInt32 documentID;
	const char* codeBase;
	const char* archive;
	Boolean mayScript;
};

class MRJPage {
public:
	MRJPage(MRJSession* session, UInt32 documentID, const char* codeBase, const char* archive, Boolean mayScript);
	MRJPage(MRJSession* session, const MRJPageAttributes& attributes);
	~MRJPage();
	
	
	UInt16 AddRef(void);
	UInt16 Release(void);
	
	JMAppletPageRef getPageRef() { return mPageRef; }
	
	UInt32 getDocumentID() { return mDocumentID; }
	const char* getCodeBase() { return mCodeBase; }
	const char* getArchive() { return mArchive; }
	Boolean getMayScript() { return mMayScript; }

	
	Boolean createContext(JMAWTContextRef* outContext,
							const JMAWTContextCallbacks * callbacks,
							JMClientData data);

    
    static MRJPage* getFirstPage(void);
    MRJPage* getNextPage(void);

private:
	void pushPage();
	void popPage();

private:
	UInt16 mRefCount;
	MRJPage* mNextPage;
	MRJSession* mSession;
	JMAppletPageRef mPageRef;
	UInt32 mDocumentID;
	char* mCodeBase;
	char* mArchive;
	Boolean mMayScript;	
};

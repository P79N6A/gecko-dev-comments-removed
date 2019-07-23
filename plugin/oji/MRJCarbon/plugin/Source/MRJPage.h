












































#include <MacTypes.h>

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
	
	UInt32 getDocumentID() { return mDocumentID; }
	const char* getCodeBase() { return mCodeBase; }
	const char* getArchive() { return mArchive; }
	Boolean getMayScript() { return mMayScript; }

    
    static MRJPage* getFirstPage(void);
    MRJPage* getNextPage(void);

private:
	void pushPage();
	void popPage();

private:
	UInt16 mRefCount;
	MRJPage* mNextPage;
	MRJSession* mSession;
	UInt32 mDocumentID;
	char* mCodeBase;
	char* mArchive;
	Boolean mMayScript;	
};

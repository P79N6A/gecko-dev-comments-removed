











































#include "MRJPage.h"
#include "MRJSession.h"

#include "StringUtils.h"

MRJPage::MRJPage(MRJSession* session, UInt32 documentID, const char* codeBase, const char* archive, Boolean mayScript)
	:	mRefCount(0), mNextPage(NULL), mSession(session), mPageRef(NULL),
		mDocumentID(documentID), mCodeBase(strdup(codeBase)), mArchive(strdup(archive)), mMayScript(mayScript)
{
	pushPage();

	if (&::JMNewAppletPage != NULL) {
		OSStatus status = ::JMNewAppletPage(&mPageRef, session->getSessionRef());
		if (status != noErr) mPageRef = NULL;
	}
}

MRJPage::MRJPage(MRJSession* session, const MRJPageAttributes& attributes)
	:	mRefCount(0), mNextPage(NULL), mSession(session), mPageRef(NULL),
		mDocumentID(attributes.documentID), mCodeBase(strdup(attributes.codeBase)),
		mArchive(strdup(attributes.archive)), mMayScript(attributes.mayScript)
{
	pushPage();

	if (&::JMNewAppletPage != NULL) {
		OSStatus status = ::JMNewAppletPage(&mPageRef, session->getSessionRef());
		if (status != noErr) mPageRef = NULL;
	}
}

MRJPage::~MRJPage()
{
	popPage();
	
	if (&::JMDisposeAppletPage != NULL && mPageRef != NULL) {
		OSStatus status = ::JMDisposeAppletPage(mPageRef);
		mPageRef = NULL;
	}
	
	if (mCodeBase != NULL) {
		delete[] mCodeBase;
		mCodeBase = NULL;
	}
	
	if (mArchive != NULL) {
		delete[] mArchive;
		mArchive = NULL;
	}
}

UInt16 MRJPage::AddRef()
{
	return (++mRefCount);
}

UInt16 MRJPage::Release()
{
	UInt16 result = --mRefCount;
	if (result == 0) {
		delete this;
	}
	return result;
}

Boolean MRJPage::createContext(JMAWTContextRef* outContext, const JMAWTContextCallbacks * callbacks, JMClientData data)
{
	OSStatus status = noErr;
	if (&::JMNewAWTContextInPage != NULL && mPageRef != NULL) {
		status = ::JMNewAWTContextInPage(outContext, mSession->getSessionRef(), mPageRef, callbacks, data);
	} else {
		status = ::JMNewAWTContext(outContext, mSession->getSessionRef(), callbacks, data);
	}
	return (status == noErr);
}

static MRJPage* thePageList = NULL;

MRJPage* MRJPage::getFirstPage()
{
	return thePageList;
}

MRJPage* MRJPage::getNextPage()
{
	return mNextPage;
}

void MRJPage::pushPage()
{
	
	mNextPage = thePageList;
	thePageList = this;
}

void MRJPage::popPage()
{
	
	MRJPage** link = &thePageList;
	MRJPage* page  = *link;
	while (page != NULL) {
		if (page == this) {
			*link = mNextPage;
			mNextPage = NULL;
			break;
		}
		link = &page->mNextPage;
		page = *link;
	}
}

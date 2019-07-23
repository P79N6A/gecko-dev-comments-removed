










































class CNsIClipBoardCmd
{	
public:

	CNsIClipBoardCmd(nsIWebBrowser* mWebBrowser);

public:
	~CNsIClipBoardCmd();

public:
	nsCOMPtr<nsIWebBrowser> qaWebBrowser ;

	void OnStartTests(UINT nMenuID);
	void OnPasteTest();
	void OnCopyTest();
	void OnSelectAllTest();
	void OnSelectNoneTest();
	void OnCutSelectionTest();
	void copyLinkLocationTest();
	void canCopySelectionTest();
	void canCutSelectionTest();
	void canPasteTest();

protected:


};
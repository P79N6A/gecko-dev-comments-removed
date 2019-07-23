









































class CSelection
{	
public:

	CSelection(nsIWebBrowser* mWebBrowser);

public:
	~CSelection();

public:
	nsCOMPtr<nsIWebBrowser> qaWebBrowser ;

	
	void OnStartTests(UINT nMenuID);
	void RunAllTests();
	void GetAnchorNode() ;
	void GetAnchorOffset() ;
	void GetFocusNode() ;
	void GetFocusOffset() ;
	void GetIsCollapsed() ;
	void GetRangecount() ;
	void GetRangeAt() ;
	void Collapse() ;
	void Extend() ;
	void CollapseToStart() ;
	void CollapseToEnd() ;
	void ContainsNode() ;
	void SelectAllChildren();
	void Addrange();
	void RemoveRange();
	void RemoveAllRanges();
	void DeleteFromDocument();
	void SelectionLanguageChange();
	void ToString();
	nsISelection * CSelection::GetSelectionObject(BOOL bShowDialog = false) ;
	
private:
protected:


};
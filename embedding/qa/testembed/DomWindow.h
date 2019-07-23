









































class CDomWindow
{
public:
	CDomWindow(nsIWebBrowser* mWebBrowser);
private:

public:
	nsCOMPtr<nsIWebBrowser> qaWebBrowser ;
	void OnStartTests(UINT nMenuID);
	void RunAllTests();
	void GetContentDOMWindow();
	void GetTop();
	void GetParent();
	void GetScrollbars();
	void GetFrames();
	void GetDocument();
	void GetSelection();
	void GetTextZoom();
	void SetTextZoom();
	void GetScrollX();
	void GetScrollY();
	void ScrollTo();
	void ScrollBy();
	void ScrollByLines();
	void ScrollByPages();
	void SizeToContent();
	nsIDOMWindow* GetDOMOWindowObject();
public:
	virtual ~CDomWindow();

protected:
};
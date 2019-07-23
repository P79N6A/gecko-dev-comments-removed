









































class CProfile
{	
public:

	CProfile(nsIWebBrowser* mWebBrowser);

public:
	~CProfile();

public:
	nsCOMPtr<nsIWebBrowser> qaWebBrowser ;

	
	void OnStartTests(UINT nMenuID);
	void RunAllTests();
	void GetProfileCount();
	void GetCurrentProfile();
	void SetCurrentProfile();
	void GetProfileList();
	void ProfileExists();
	void CreateNewProfile();
	void RenameProfile();
	void DeleteProfile();
	void CloneProfile();
	void ShutDownCurrentProfile();

private:
protected:


};
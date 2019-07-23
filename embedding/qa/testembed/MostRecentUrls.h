





































class CMostRecentUrls {
public:
    enum _maxUrls {
        MAX_URLS = 16
    };

    CMostRecentUrls();
    virtual ~CMostRecentUrls();

    char * GetURL(int aInx);
    void AddURL(const char * aURL);
    inline int GetNumURLs() { return mNumURLs; }
    FILE * CMostRecentUrls::GetFD(const char * aMode);

protected:
    char * mURLs[MAX_URLS];
    int    mNumURLs;
};

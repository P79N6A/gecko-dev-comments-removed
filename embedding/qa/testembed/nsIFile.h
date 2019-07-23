












































class CNsIFile
{	
public:
	CNsIFile();


public:
	~CNsIFile();

public:
	void OnStartTests(UINT nMenuID);
	void RunAllTests(nsILocalFile*, nsILocalFile *);
	void InitWithPathTest(nsILocalFile*, PRInt16);
	void AppendRelativePathTest(nsILocalFile*, PRInt16);
	void FileCreateTest(nsILocalFile*, PRInt16);
	void FileExistsTest(nsILocalFile*, PRInt16);
	void FileCopyTest(nsILocalFile*, nsILocalFile *, PRInt16);	
	void FileMoveTest(nsILocalFile*, nsILocalFile *, PRInt16);	

protected:


};
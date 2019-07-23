





































#include "txStandaloneXSLTProcessor.h"
#include "nsXPCOM.h"
#include <fstream.h>
#include "nsDoubleHashtable.h"
#include "nsIComponentManager.h"
#include "nsILocalFile.h"
#include "nsISimpleEnumerator.h"
#include "nsString.h"
#include "nsVoidArray.h"
#include "prenv.h"
#include "prsystem.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"

#ifdef NS_TRACE_MALLOC
#include "nsTraceMalloc.h"
#endif
#ifdef MOZ_JPROF
#include "jprof.h"
#endif




void printHelp()
{
  cerr << "testXalan [-o output-file] [category]*" << endl << endl;
  cerr << "Options:";
  cerr << endl << endl;
  cerr << "\t-o  specify output file (default: write to stdout)";
  cerr << endl << endl;
  cerr << "\t Specify XALAN_DIR in your environement." << endl;
  cerr << endl;
}




class txRDFOut
{
public:
    explicit txRDFOut(ostream* aOut)
        : mOut(aOut), mSuccess(0), mFail(0), mParent(nsnull)
    {
    }
    explicit txRDFOut(const nsACString& aName, txRDFOut* aParent)
        : mName(aName), mOut(aParent->mOut), mSuccess(0), mFail(0),
          mParent(aParent)
    {
    }
    ~txRDFOut()
    {
        *mOut << "  <RDF:Description about=\"urn:x-buster:conf" <<
            mName.get() <<
            "\">\n" <<
            "    <NC:orig_succCount NC:parseType=\"Integer\">" <<
            mSuccess <<
            "</NC:orig_succCount>\n" <<
            "    <NC:orig_failCount NC:parseType=\"Integer\">" <<
            mFail <<
            "</NC:orig_failCount>\n" <<
            "  </RDF:Description>" << endl;
    }

    void feed(const nsACString& aTest, PRBool aSuccess)
    {
        *mOut << "  <RDF:Description about=\"urn:x-buster:" <<
            PromiseFlatCString(aTest).get() <<
            "\"\n                   NC:orig_succ=\"";
        if (aSuccess) {
            *mOut << "yes";
            succeeded();
        }
        else {
            *mOut << "no";
            failed();
        }
        *mOut << "\" />\n";
    }

    void succeeded()
    {
        if (mParent)
            mParent->succeeded();
        ++mSuccess;
    }
    void failed()
    {
        if (mParent)
            mParent->failed();
        ++mFail;
    }
private:
    nsCAutoString mName;
    ostream* mOut;
    PRUint32 mSuccess, mFail;
    txRDFOut* mParent;
};

static void
readToString(istream& aIstream, nsACString& aString)
{
    static char buffer[1024];
    int read = 0;
    do {
        aIstream.read(buffer, 1024);
        read = aIstream.gcount();
        aString.Append(Substring(buffer, buffer + read));
    } while (!aIstream.eof());
}






static nsresult
setupXalan(const char* aPath, nsIFile** aConf, nsIFile** aConfGold,
           nsIFile** aTemp)
{
    nsresult rv;
    nsCOMPtr<nsILocalFile> conf(do_CreateInstance(NS_LOCAL_FILE_CONTRACTID,
                                                  &rv));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr <nsIFile> tmpFile;
    rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tmpFile));
    NS_ENSURE_SUCCESS(rv, rv);
    tmpFile->Append(NS_LITERAL_STRING("xalan.out"));
    rv = tmpFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
    rv = conf->InitWithNativePath(nsDependentCString(aPath));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIFile> gold;
    rv = conf->Clone(getter_AddRefs(gold));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = conf->Append(NS_LITERAL_STRING("conf"));
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool isDir;
    rv = conf->IsDirectory(&isDir);
    if (NS_FAILED(rv) || !isDir) {
        return NS_ERROR_FILE_NOT_DIRECTORY;
    }
    rv = gold->Append(NS_LITERAL_STRING("conf-gold"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = gold->IsDirectory(&isDir);
    if (NS_FAILED(rv) || !isDir || !conf || !gold) {
        return NS_ERROR_FILE_NOT_DIRECTORY;
    }
    
    *aConf = conf;
    NS_ADDREF(*aConf);
    *aConfGold = gold;
    NS_ADDREF(*aConfGold);
    *aTemp = tmpFile;
    NS_ADDREF(*aTemp);
    return NS_OK;
}




void runCategory(nsIFile* aConfCat, nsIFile* aGoldCat, nsIFile* aRefTmp,
                 txRDFOut* aOut)
{
    nsresult rv;
    
    nsCOMPtr<nsIFile> conf, gold;
    aConfCat->Clone(getter_AddRefs(conf));
    aGoldCat->Clone(getter_AddRefs(gold));
    nsCAutoString catName, refTmp;
    conf->GetNativeLeafName(catName);
    aRefTmp->GetNativePath(refTmp);
    txRDFOut results(catName, aOut);
    nsCOMPtr<nsISimpleEnumerator> tests;
    rv = conf->GetDirectoryEntries(getter_AddRefs(tests));
    if (NS_FAILED(rv))
        return;
    PRBool hasMore, isFile;
    nsCAutoString leaf;
    NS_NAMED_LITERAL_CSTRING(xsl, ".xsl");
    while (NS_SUCCEEDED(tests->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsILocalFile> test;
        tests->GetNext(getter_AddRefs(test));
        test->GetNativeLeafName(leaf);
        if (xsl.Equals(Substring(leaf, leaf.Length()-4, 4))) {
            
            nsAFlatCString::char_iterator start, ext;
            leaf.BeginWriting(start);
            leaf.EndWriting(ext);
            ext -= 2;
            
            *ext = 'm'; 
            nsCOMPtr<nsIFile> source;
            conf->Clone(getter_AddRefs(source));
            rv = source->AppendNative(leaf);
            if (NS_SUCCEEDED(rv) && NS_SUCCEEDED(source->IsFile(&isFile)) &&
                isFile) {
                nsCOMPtr<nsIFile> reference;
                gold->Clone(getter_AddRefs(reference));
                
                --ext;
                nsCharTraits<char>::copy(ext, "out", 3);
                rv = reference->AppendNative(leaf);
                if (NS_SUCCEEDED(rv) &&
                    NS_SUCCEEDED(reference->IsFile(&isFile)) &&
                    isFile) {
                    nsCAutoString src, style, refPath;
                    test->GetNativePath(style);
                    source->GetNativePath(src);
                    reference->GetNativePath(refPath);
                    if (PR_GetDirectorySeparator() =='\\') {
                        src.ReplaceChar('\\','/');
                        style.ReplaceChar('\\','/');
                        refPath.ReplaceChar('\\','/');
                    }
                    SimpleErrorObserver obs;
                    txStandaloneXSLTProcessor proc;
                    fstream result(refTmp.get(),
                                   ios::in | ios::out | ios::trunc);
                    rv = proc.transform(src, style, result, obs);
                    PRBool success = PR_FALSE;
                    if (NS_SUCCEEDED(rv)) {
                        result.flush();
                        PRInt64 resultSize, refSize;
                        aRefTmp->GetFileSize(&resultSize);
                        reference->GetFileSize(&refSize);
                        result.seekg(0);
                        int toread = (int)resultSize;
                        nsCString resContent, refContent;
                        resContent.SetCapacity(toread);
                        readToString(result, resContent);
                        result.close();
                        ifstream refStream(refPath.get());
                        toread = (int)refSize;
                        refContent.SetCapacity(toread);
                        readToString(refStream, refContent);
                        refStream.close();
                        success = resContent.Equals(refContent);
                    }
                    ext--;
                    results.feed(Substring(start, ext), success);
                }
            }
        }
    }
}



int main(int argc, char** argv)
{
#ifdef NS_TRACE_MALLOC
    NS_TraceMallocStartupArgs(argc, argv);
#endif
#ifdef MOZ_JPROF
    setupProfilingStuff();
#endif
    char* xalan = PR_GetEnv("XALAN_DIR");
    if (!xalan) {
        printHelp();
        return 1;
    }
    nsresult rv;
    rv = NS_InitXPCOM2(nsnull, nsnull, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> conf, gold, resFile;
    rv = setupXalan(xalan, getter_AddRefs(conf), getter_AddRefs(gold),
                    getter_AddRefs(resFile));
    if (NS_FAILED(rv)) {
        NS_ShutdownXPCOM(nsnull);
        printHelp();
        return -1;
    }

    
    ostream* resultOutput = &cout;
    ofstream resultFileStream;

    int argn = 1;
    
    while (argn < argc) {
        nsDependentCString opt(argv[argn]);
        if (!Substring(opt, 0, 2).EqualsLiteral("--")) {
            break;
        }
        ++argn;
    }
    if (argn < argc) {
        nsDependentCString opt(argv[argn]);
        if (Substring(opt, 0, 2).EqualsLiteral("-o")) {
            if (opt.Length() > 2) {
                const nsAFlatCString& fname = 
                    PromiseFlatCString(Substring(opt, 2, opt.Length()-2));
                resultFileStream.open(fname.get(), ios::out);
            }
            else {
                ++argn;
                if (argn < argc) {
                    resultFileStream.open(argv[argn], ios::out);
                }
            }
            if (!resultFileStream) {
                cerr << "error opening output file" << endl;
                PRBool exists;
                if (NS_SUCCEEDED(resFile->Exists(&exists)) && exists)
                    resFile->Remove(PR_FALSE);
                NS_ShutdownXPCOM(nsnull);
                return -1;
            }
            ++argn;
            resultOutput = &resultFileStream;
        }
    }

    if (!txXSLTProcessor::init()) {
        PRBool exists;
        if (NS_SUCCEEDED(resFile->Exists(&exists)) && exists)
            resFile->Remove(PR_FALSE);
        NS_ShutdownXPCOM(nsnull);
        return 1;
    }

    *resultOutput << "<?xml version=\"1.0\"?>\n" << 
        "<RDF:RDF xmlns:NC=\"http://home.netscape.com/NC-rdf#\"\n" <<
        "         xmlns:RDF=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">" << endl;

    txRDFOut* rdfOut = new txRDFOut(resultOutput);
    nsCOMPtr<nsIFile> tempFile;
    if (argn < argc) {
        
        while (argn < argc) {
            nsDependentCString cat(argv[argn++]);
            rv = conf->AppendNative(cat);
            if (NS_SUCCEEDED(rv)) {
                rv = gold->AppendNative(cat);
                if (NS_SUCCEEDED(rv)) {
                    runCategory(conf, gold, resFile, rdfOut);
                    rv = gold->GetParent(getter_AddRefs(tempFile));
                    NS_ASSERTION(NS_SUCCEEDED(rv), "can't go back?");
                    gold = tempFile;
                }
                rv = conf->GetParent(getter_AddRefs(tempFile));
                NS_ASSERTION(NS_SUCCEEDED(rv), "can't go back?");
                conf = tempFile;
            }
        }
    }
    else {
        
        nsCOMPtr<nsISimpleEnumerator> cats;
        rv = conf->GetDirectoryEntries(getter_AddRefs(cats));
        PRBool hasMore, isDir;
        nsCAutoString leaf;
        while (NS_SUCCEEDED(cats->HasMoreElements(&hasMore)) && hasMore) {
            nsCOMPtr<nsILocalFile> cat;
            cats->GetNext(getter_AddRefs(cat));
            rv = cat->IsDirectory(&isDir);
            if (NS_SUCCEEDED(rv) && isDir) {
                rv = cat->GetNativeLeafName(leaf);
                if (NS_SUCCEEDED(rv) && 
                    !leaf.EqualsLiteral("CVS")) {
                    rv = gold->AppendNative(leaf);
                    if (NS_SUCCEEDED(rv)) {
                        runCategory(cat, gold, resFile, rdfOut);
                        rv = gold->GetParent(getter_AddRefs(tempFile));
                        gold = tempFile;
                    }
                }
            }
        }
    }
    delete rdfOut;
    rdfOut = nsnull;
    *resultOutput << "</RDF:RDF>" << endl;
    PRBool exists;
    if (NS_SUCCEEDED(resFile->Exists(&exists)) && exists)
        resFile->Remove(PR_FALSE);
    resultFileStream.close();
    txXSLTProcessor::shutdown();
    rv = NS_ShutdownXPCOM(nsnull);
#ifdef NS_TRACE_MALLOC
    NS_TraceMallocShutdown();
#endif
    NS_ENSURE_SUCCESS(rv, rv);
    return 0;
}

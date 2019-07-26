






#ifndef XPCJSMemoryReporter_h
#define XPCJSMemoryReporter_h

class nsISupports;
class nsIMemoryMultiReporterCallback;

namespace xpc {


typedef nsDataHashtable<nsUint64HashKey, nsCString> WindowPaths;




class JSMemoryMultiReporter
{
public:
    static nsresult CollectReports(WindowPaths *windowPaths,
                                   WindowPaths *topWindowPaths,
                                   nsIMemoryMultiReporterCallback *cb,
                                   nsISupports *closure);

    static nsresult GetExplicitNonHeap(int64_t *n);
};

}

#endif

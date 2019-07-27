





#ifndef XPCJSMemoryReporter_h
#define XPCJSMemoryReporter_h

class nsISupports;
class nsIMemoryReporterCallback;

namespace xpc {


typedef nsDataHashtable<nsUint64HashKey, nsCString> WindowPaths;




class JSReporter
{
public:
    static nsresult CollectReports(WindowPaths* windowPaths,
                                   WindowPaths* topWindowPaths,
                                   nsIMemoryReporterCallback* cb,
                                   nsISupports* closure,
                                   bool anonymize);
};

} 

#endif

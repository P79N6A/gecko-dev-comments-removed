

Cu.import("resource://gre/modules/Services.jsm");

function completeTest(request, data, ctx)
{
    do_test_finished();
}

function run_test()
{
    var ios = Components.classes["@mozilla.org/network/io-service;1"].
                         getService(Components.interfaces.nsIIOService);
    var chan = ios.newChannel2("http://localhost:80000/",
                               "",
                               null,
                               null,      
                               Services.scriptSecurityManager.getSystemPrincipal(),
                               null,      
                               Ci.nsILoadInfo.SEC_NORMAL,
                               Ci.nsIContentPolicy.TYPE_OTHER);
    var httpChan = chan.QueryInterface(Components.interfaces.nsIHttpChannel);
    httpChan.asyncOpen(new ChannelListener(completeTest,
                                           httpChan, CL_EXPECT_FAILURE), null);
    do_test_pending();
}


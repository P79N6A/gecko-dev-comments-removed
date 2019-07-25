

function completeTest(request, data, ctx)
{
    do_test_finished();
}

function run_test()
{
    var ios = Components.classes["@mozilla.org/network/io-service;1"].
                         getService(Components.interfaces.nsIIOService);
    var chan = ios.newChannel("http://localhost:80000/", "", null);
    var httpChan = chan.QueryInterface(Components.interfaces.nsIHttpChannel);
    httpChan.asyncOpen(new ChannelListener(completeTest,
                                           httpChan, CL_EXPECT_FAILURE), null);
    do_test_pending();
}







const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");

var httpserver = new HttpServer();
var iteration = 0;

function setupChannel(suffix)
{
    var ios =
        Components.classes["@mozilla.org/network/io-service;1"]
        .getService(Ci.nsIIOService);
    var chan = ios.newChannel("http://localhost:4444" + suffix, "", null);
    var httpChan = chan.QueryInterface(Components.interfaces.nsIHttpChannel);
    httpChan.requestMethod = "GET";
    return httpChan;
}

function checkValueAndTrigger(request, data, ctx)
{
    do_check_eq("Ok", data);
    httpserver.stop(do_test_finished);
}

function run_test()
{
    httpserver.registerPathHandler("/redirect1", redirectHandler1);
    httpserver.registerPathHandler("/redirect2", redirectHandler2);
    httpserver.start(4444);

    
    evict_cache_entries();

    
    var channel = setupChannel("/redirect1");
    channel.asyncOpen(new ChannelListener(checkValueAndTrigger, null), null);

    do_test_pending();
}

function redirectHandler1(metadata, response)
{
    
	if (iteration < 1) {
	    response.setStatusLine(metadata.httpVersion, 302, "Found");
	    response.setHeader("Cache-Control", "max-age=600", false);
	    response.setHeader("Location", "/redirect2", false);

	
    } else {
	    response.setStatusLine(metadata.httpVersion, 200, "Ok");
	    response.setHeader("Cache-Control", "max-age=600", false);
        response.setHeader("Content-Type", "text/plain");
        response.bodyOutputStream.write("Ok", "Ok".length);
    }
	iteration += 1;
}

function redirectHandler2(metadata, response)
{
    response.setStatusLine(metadata.httpVersion, 302, "Found");
    response.setHeader("Cache-Control", "max-age=600", false);
    response.setHeader("Location", "/redirect1", false);
}

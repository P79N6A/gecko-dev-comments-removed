const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");
var httpserver = new HttpServer();

var ios;













var listener_3 = {
    
    

    QueryInterface: function(iid) {
	if (iid.equals(Components.interfaces.nsIStreamListener) ||
            iid.equals(Components.interfaces.nsIRequestObserver) ||
            iid.equals(Components.interfaces.nsISupports))
	    return this;
	throw Components.results.NS_ERROR_NO_INTERFACE;
    },

    onStartRequest: function test_onStartR(request, ctx) {},
    
    onDataAvailable: function test_ODA(request, cx, inputStream,
                                       offset, count) {
	var data = new BinaryInputStream(inputStream).readByteArray(count);
      
	do_check_eq(data[0], "B".charCodeAt(0));
    },

    onStopRequest: function test_onStopR(request, ctx, status) {
	httpserver.stop(do_test_finished);
    }
};

var listener_2 = {
    
    

    QueryInterface: function(iid) {
	if (iid.equals(Components.interfaces.nsIStreamListener) ||
            iid.equals(Components.interfaces.nsIRequestObserver) ||
            iid.equals(Components.interfaces.nsISupports))
	    return this;
	throw Components.results.NS_ERROR_NO_INTERFACE;
    },

    onStartRequest: function test_onStartR(request, ctx) {},
    
    onDataAvailable: function test_ODA(request, cx, inputStream,
                                       offset, count) {
	var data = new BinaryInputStream(inputStream).readByteArray(count);
      
	
	
	
	do_check_eq(data[0], "A".charCodeAt(0));
    },

    onStopRequest: function test_onStopR(request, ctx, status) {
	var channel = request.QueryInterface(Ci.nsIHttpChannel);

	var chan = ios.newChannel("http://localhost:4444/test1", "", null);
	chan.asyncOpen(listener_3, null);
    }
};

var listener_1 = {
    
    

    QueryInterface: function(iid) {
	if (iid.equals(Components.interfaces.nsIStreamListener) ||
            iid.equals(Components.interfaces.nsIRequestObserver) ||
            iid.equals(Components.interfaces.nsISupports))
	    return this;
	throw Components.results.NS_ERROR_NO_INTERFACE;
    },

    onStartRequest: function test_onStartR(request, ctx) {},
    
    onDataAvailable: function test_ODA(request, cx, inputStream,
                                       offset, count) {
	var data = new BinaryInputStream(inputStream).readByteArray(count);
	do_check_eq(data[0], "A".charCodeAt(0));
    },

    onStopRequest: function test_onStopR(request, ctx, status) {
	var channel = request.QueryInterface(Ci.nsIHttpChannel);

	var chan = ios.newChannel("http://localhost:4444/test1", "", null);
	chan.asyncOpen(listener_2, null);
    }
};

function run_test() {
    do_get_profile();
    ios = Cc["@mozilla.org/network/io-service;1"]
            .getService(Ci.nsIIOService);

    evict_cache_entries();

    httpserver.registerPathHandler("/test1", handler);
    httpserver.start(4444);

    var chan = ios.newChannel("http://localhost:4444/test1", "", null);
    chan.asyncOpen(listener_1, null);

    do_test_pending();
}

var iter=0;
function handler(metadata, response) {
    iter++;
    if (metadata.hasHeader("If-Modified-Since")) {
	response.setStatusLine(metadata.httpVersion, 304, "Not Modified");
	response.setHeader("Last-Modified", "Tue, 15 Nov 1994 12:45:26 GMT", false);
    }    
    else {
	response.setStatusLine(metadata.httpVersion, 200, "OK");
	response.setHeader("Cache-Control", "max-age=0", false)
	if (iter == 1) {
	    
	    response.setHeader("Last-Modified", "Wed, 16 Nov 1994 00:00:00 GMT", false);
	    response.bodyOutputStream.write("A", 1);
	}
	if (iter == 3) {
	    
	    response.setHeader("Last-Modified", "Tue, 15 Nov 1994 12:45:26 GMT", false);
	    response.bodyOutputStream.write("B", 1);
	}
    }
}

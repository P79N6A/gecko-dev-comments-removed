do_load_httpd_js();
var httpserver = new nsHttpServer();
var cacheService;
var ios;













var listener_3 = {
    
    

    onStartRequest: function test_onStartR(request, ctx) {},
    
    onDataAvailable: function test_ODA(request, cx, inputStream,
                                       offset, count) {
	var data = new BinaryInputStream(inputStream).readByteArray(count);
      
	
	do_check_eq(data, 66);
    },

    onStopRequest: function test_onStopR(request, ctx, status) {
	httpserver.stop(do_test_finished);
    }
};

var listener_2 = {
    
    

    onStartRequest: function test_onStartR(request, ctx) {},
    
    onDataAvailable: function test_ODA(request, cx, inputStream,
                                       offset, count) {
	var data = new BinaryInputStream(inputStream).readByteArray(count);
      
	
	
	
	do_check_eq(data, 65);
    },

    onStopRequest: function test_onStopR(request, ctx, status) {
	var channel = request.QueryInterface(Ci.nsIHttpChannel);

	var chan = ios.newChannel("http://localhost:4444/test1", "", null);
	var httpChan = chan.QueryInterface(Ci.nsIHttpChannel);
	httpChan.requestMethod = "GET";
	httpChan.asyncOpen(listener_3, null);
    }
};

var listener_1 = {
    
    

    onStartRequest: function test_onStartR(request, ctx) {},
    
    onDataAvailable: function test_ODA(request, cx, inputStream,
                                       offset, count) {
	var data = new BinaryInputStream(inputStream).readByteArray(count);
	do_check_eq(data, 65);
    },

    onStopRequest: function test_onStopR(request, ctx, status) {
	var channel = request.QueryInterface(Ci.nsIHttpChannel);

	var chan = ios.newChannel("http://localhost:4444/test1", "", null);
	var httpChan = chan.QueryInterface(Ci.nsIHttpChannel);
	httpChan.requestMethod = "GET";
	httpChan.asyncOpen(listener_2, null);
    }
};

function run_test() {
    do_get_profile();
    cacheService = Cc["@mozilla.org/network/cache-service;1"].
        getService(Ci.nsICacheService);
    ios = Cc["@mozilla.org/network/io-service;1"]
            .getService(Ci.nsIIOService);

    cacheService.evictEntries(Ci.nsICache.STORE_ANYWHERE);

    httpserver.registerPathHandler("/test1", handler);
    httpserver.start(4444);

    var chan = ios.newChannel("http://localhost:4444/test1", "", null);
    var httpChan = chan.QueryInterface(Ci.nsIHttpChannel);
    httpChan.requestMethod = "GET";
    httpChan.asyncOpen(listener_1, null);

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

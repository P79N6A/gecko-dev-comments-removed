












const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://testing-common/httpd.js");

var httpserv;

function setupChannel(path) {
    var ios =
        Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    return chan = ios.newChannel(path, "", null)
                                .QueryInterface(Ci.nsIHttpChannel);
}


function InitialListener() { }
InitialListener.prototype = {
    onStartRequest: function(request, context) { },
    onStopRequest: function(request, context, status) {
        do_check_eq(1, numberOfCLHandlerCalls);
        do_execute_soon(function() {
            var channel = setupChannel("http://localhost:4444/post");
            channel.requestMethod = "post";
            channel.asyncOpen(new RedirectingListener(), null);
        });
    }
};


function RedirectingListener() { }
RedirectingListener.prototype = {
    onStartRequest: function(request, context) { },
    onStopRequest: function(request, context, status) {
        do_check_eq(1, numberOfHandlerCalls);
        do_execute_soon(function() {
            var channel = setupChannel("http://localhost:4444/post");
            channel.requestMethod = "post";
            channel.asyncOpen(new VerifyingListener(), null);
        });
    }
};



function VerifyingListener() { }
VerifyingListener.prototype = {
    onStartRequest: function(request, context) { },
    onStopRequest: function(request, context, status) {
        do_check_eq(2, numberOfHandlerCalls);
        var channel = setupChannel("http://localhost:4444/cl");
        channel.asyncOpen(new FinalListener(), null);
    }
};



function FinalListener() { }
FinalListener.prototype = {
    onStartRequest: function(request, context) { },
    onStopRequest: function(request, context, status) {
        do_check_eq(2, numberOfCLHandlerCalls);
        httpserv.stop(do_test_finished);
    }
};

function run_test() {
  httpserv = new HttpServer();
  httpserv.registerPathHandler("/cl", content_location);
  httpserv.registerPathHandler("/post", post_target);
  httpserv.registerPathHandler("/redirect", redirect_target);
  httpserv.start(4444);

  
  evict_cache_entries();

  
  var channel = setupChannel("http://localhost:4444/cl");
  channel.asyncOpen(new InitialListener(), null);

  do_test_pending();
}

var numberOfCLHandlerCalls = 0;
function content_location(metadata, response) {
    numberOfCLHandlerCalls++;
    response.setStatusLine(metadata.httpVersion, 200, "Ok");
    response.setHeader("Cache-Control", "max-age=360000", false);
}

function post_target(metadata, response) {
    response.setStatusLine(metadata.httpVersion, 301, "Moved Permanently");
    response.setHeader("Location", "/redirect", false);
    response.setHeader("Content-Location", "/cl", false);
    response.setHeader("Cache-Control", "max-age=360000", false);
}

var numberOfHandlerCalls = 0;
function redirect_target(metadata, response) {
    numberOfHandlerCalls++;
    response.setStatusLine(metadata.httpVersion, 200, "Ok");
    response.setHeader("Cache-Control", "max-age=360000", false);
}

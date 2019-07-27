


"use strict";

Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/rest.js");
Cu.import("resource://services-common/utils.js");

function run_test() {
  Log.repository.getLogger("Services.Common.RESTRequest").level =
    Log.Level.Trace;
  initTestLogging("Trace");

  run_next_test();
}





add_test(function test_invalid_uri() {
  do_check_throws(function() {
    new RESTRequest("an invalid URI");
  }, Cr.NS_ERROR_MALFORMED_URI);
  run_next_test();
});




add_test(function test_attributes() {
  let uri = "http://foo.com/bar/baz";
  let request = new RESTRequest(uri);

  do_check_true(request.uri instanceof Ci.nsIURI);
  do_check_eq(request.uri.spec, uri);
  do_check_eq(request.response, null);
  do_check_eq(request.status, request.NOT_SENT);
  let expectedLoadFlags = Ci.nsIRequest.LOAD_BYPASS_CACHE |
                          Ci.nsIRequest.INHIBIT_CACHING |
                          Ci.nsIRequest.LOAD_ANONYMOUS;
  do_check_eq(request.loadFlags, expectedLoadFlags);

  run_next_test();
});





add_test(function test_proxy_auth_redirect() {
  let pacFetched = false;
  function pacHandler(metadata, response) {
    pacFetched = true;
    let body = 'function FindProxyForURL(url, host) { return "DIRECT"; }';
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "application/x-ns-proxy-autoconfig", false);
    response.bodyOutputStream.write(body, body.length);
  }

  let fetched = false;
  function original(metadata, response) {
    fetched = true;
    let body = "TADA!";
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.bodyOutputStream.write(body, body.length);
  }

  let server = httpd_setup({
    "/original": original,
    "/pac3":     pacHandler
  });
  PACSystemSettings.PACURI = server.baseURI + "/pac3";
  installFakePAC();

  let res = new RESTRequest(server.baseURI + "/original");
  res.get(function (error) {
    do_check_true(pacFetched);
    do_check_true(fetched);
    do_check_true(!error);
    do_check_true(this.response.success);
    do_check_eq("TADA!", this.response.body);
    uninstallFakePAC();
    server.stop(run_next_test);
  });
});






add_test(function test_forbidden_port() {
  let request = new RESTRequest("http://localhost:6000/");
  request.get(function(error) {
    if (!error) {
      do_throw("Should have got an error.");
    }
    do_check_eq(error.result, Components.results.NS_ERROR_PORT_ACCESS_NOT_ALLOWED);
    run_next_test();
  });
});




add_test(function test_simple_get() {
  let handler = httpd_handler(200, "OK", "Huzzah!");
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest(server.baseURI + "/resource").get(function (error) {
    do_check_eq(error, null);

    do_check_eq(this.status, this.COMPLETED);
    do_check_true(this.response.success);
    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "Huzzah!");

    server.stop(run_next_test);
  });
  do_check_eq(request.status, request.SENT);
  do_check_eq(request.method, "GET");
});




add_test(function test_get() {
  let handler = httpd_handler(200, "OK", "Huzzah!");
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest(server.baseURI + "/resource");
  do_check_eq(request.status, request.NOT_SENT);

  request.onProgress = request.onComplete = function () {
    do_throw("This function should have been overwritten!");
  };

  let onProgress_called = false;
  function onProgress() {
    onProgress_called = true;
    do_check_eq(this.status, request.IN_PROGRESS);
    do_check_true(this.response.body.length > 0);

    do_check_true(!!(this.channel.loadFlags & Ci.nsIRequest.LOAD_BYPASS_CACHE));
    do_check_true(!!(this.channel.loadFlags & Ci.nsIRequest.INHIBIT_CACHING));
  };

  function onComplete(error) {
    do_check_eq(error, null);

    do_check_eq(this.status, this.COMPLETED);
    do_check_true(this.response.success);
    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "Huzzah!");
    do_check_eq(handler.request.method, "GET");

    do_check_true(onProgress_called);
    CommonUtils.nextTick(function () {
      do_check_eq(request.onComplete, null);
      do_check_eq(request.onProgress, null);
      server.stop(run_next_test);
    });
  };

  do_check_eq(request.get(onComplete, onProgress), request);
  do_check_eq(request.status, request.SENT);
  do_check_eq(request.method, "GET");
  do_check_throws(function () {
    request.get();
  });
});




add_test(function test_get_utf8() {
  let response = "Hello World or Καλημέρα κόσμε or こんにちは 世界";

  let contentType = "text/plain";
  let charset = true;
  let charsetSuffix = "; charset=UTF-8";

  let server = httpd_setup({"/resource": function(req, res) {
    res.setStatusLine(req.httpVersion, 200, "OK");
    res.setHeader("Content-Type", contentType + (charset ? charsetSuffix : ""));

    let converter = Cc["@mozilla.org/intl/converter-output-stream;1"]
                    .createInstance(Ci.nsIConverterOutputStream);
    converter.init(res.bodyOutputStream, "UTF-8", 0, 0x0000);
    converter.writeString(response);
    converter.close();
  }});

  
  let request1 = new RESTRequest(server.baseURI + "/resource");
  request1.get(function(error) {
    do_check_null(error);

    do_check_eq(request1.response.status, 200);
    do_check_eq(request1.response.body, response);
    do_check_eq(request1.response.headers["content-type"],
                contentType + charsetSuffix);

    
    charset = false;
    let request2 = new RESTRequest(server.baseURI + "/resource");
    request2.get(function(error) {
      do_check_null(error);

      do_check_eq(request2.response.status, 200);
      do_check_eq(request2.response.body, response);
      do_check_eq(request2.response.headers["content-type"], contentType);
      do_check_eq(request2.response.charset, "utf-8");

      server.stop(run_next_test);
    });
  });
});




add_test(function test_charsets() {
  let response = "Hello World, I can't speak Russian";

  let contentType = "text/plain";
  let charset = true;
  let charsetSuffix = "; charset=us-ascii";

  let server = httpd_setup({"/resource": function(req, res) {
    res.setStatusLine(req.httpVersion, 200, "OK");
    res.setHeader("Content-Type", contentType + (charset ? charsetSuffix : ""));

    let converter = Cc["@mozilla.org/intl/converter-output-stream;1"]
                    .createInstance(Ci.nsIConverterOutputStream);
    converter.init(res.bodyOutputStream, "us-ascii", 0, 0x0000);
    converter.writeString(response);
    converter.close();
  }});

  
  let request1 = new RESTRequest(server.baseURI + "/resource");
  request1.charset = "not-a-charset";
  request1.get(function(error) {
    do_check_null(error);

    do_check_eq(request1.response.status, 200);
    do_check_eq(request1.response.body, response);
    do_check_eq(request1.response.headers["content-type"],
                contentType + charsetSuffix);
    do_check_eq(request1.response.charset, "us-ascii");

    
    charset = false;
    let request2 = new RESTRequest(server.baseURI + "/resource");
    request2.charset = "us-ascii";
    request2.get(function(error) {
      do_check_null(error);

      do_check_eq(request2.response.status, 200);
      do_check_eq(request2.response.body, response);
      do_check_eq(request2.response.headers["content-type"], contentType);
      do_check_eq(request2.response.charset, "us-ascii");

      server.stop(run_next_test);
    });
  });
});




function check_posting_data(method) {
  let funcName = method.toLowerCase();
  let handler = httpd_handler(200, "OK", "Got it!");
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest(server.baseURI + "/resource");
  do_check_eq(request.status, request.NOT_SENT);

  request.onProgress = request.onComplete = function () {
    do_throw("This function should have been overwritten!");
  };

  let onProgress_called = false;
  function onProgress() {
    onProgress_called = true;
    do_check_eq(this.status, request.IN_PROGRESS);
    do_check_true(this.response.body.length > 0);
  };

  function onComplete(error) {
    do_check_eq(error, null);

    do_check_eq(this.status, this.COMPLETED);
    do_check_true(this.response.success);
    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "Got it!");

    do_check_eq(handler.request.method, method);
    do_check_eq(handler.request.body, "Hullo?");
    do_check_eq(handler.request.getHeader("Content-Type"), "text/plain");

    do_check_true(onProgress_called);
    CommonUtils.nextTick(function () {
      do_check_eq(request.onComplete, null);
      do_check_eq(request.onProgress, null);
      server.stop(run_next_test);
    });
  };

  do_check_eq(request[funcName]("Hullo?", onComplete, onProgress), request);
  do_check_eq(request.status, request.SENT);
  do_check_eq(request.method, method);
  do_check_throws(function () {
    request[funcName]("Hai!");
  });
}




add_test(function test_patch() {
  check_posting_data("PATCH");
});




add_test(function test_put() {
  check_posting_data("PUT");
});




add_test(function test_post() {
  check_posting_data("POST");
});




add_test(function test_delete() {
  let handler = httpd_handler(200, "OK", "Got it!");
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest(server.baseURI + "/resource");
  do_check_eq(request.status, request.NOT_SENT);

  request.onProgress = request.onComplete = function () {
    do_throw("This function should have been overwritten!");
  };

  let onProgress_called = false;
  function onProgress() {
    onProgress_called = true;
    do_check_eq(this.status, request.IN_PROGRESS);
    do_check_true(this.response.body.length > 0);
  };

  function onComplete(error) {
    do_check_eq(error, null);

    do_check_eq(this.status, this.COMPLETED);
    do_check_true(this.response.success);
    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "Got it!");
    do_check_eq(handler.request.method, "DELETE");

    do_check_true(onProgress_called);
    CommonUtils.nextTick(function () {
      do_check_eq(request.onComplete, null);
      do_check_eq(request.onProgress, null);
      server.stop(run_next_test);
    });
  };

  do_check_eq(request.delete(onComplete, onProgress), request);
  do_check_eq(request.status, request.SENT);
  do_check_eq(request.method, "DELETE");
  do_check_throws(function () {
    request.delete();
  });
});




add_test(function test_get_404() {
  let handler = httpd_handler(404, "Not Found", "Cannae find it!");
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest(server.baseURI + "/resource");
  request.get(function (error) {
    do_check_eq(error, null);

    do_check_eq(this.status, this.COMPLETED);
    do_check_false(this.response.success);
    do_check_eq(this.response.status, 404);
    do_check_eq(this.response.body, "Cannae find it!");

    server.stop(run_next_test);
  });
});





add_test(function test_put_json() {
  let handler = httpd_handler(200, "OK");
  let server = httpd_setup({"/resource": handler});

  let sample_data = {
    some: "sample_data",
    injson: "format",
    number: 42
  };
  let request = new RESTRequest(server.baseURI + "/resource");
  request.put(sample_data, function (error) {
    do_check_eq(error, null);

    do_check_eq(this.status, this.COMPLETED);
    do_check_true(this.response.success);
    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "");

    do_check_eq(handler.request.method, "PUT");
    do_check_eq(handler.request.body, JSON.stringify(sample_data));
    do_check_eq(handler.request.getHeader("Content-Type"), "text/plain");

    server.stop(run_next_test);
  });
});





add_test(function test_post_json() {
  let handler = httpd_handler(200, "OK");
  let server = httpd_setup({"/resource": handler});

  let sample_data = {
    some: "sample_data",
    injson: "format",
    number: 42
  };
  let request = new RESTRequest(server.baseURI + "/resource");
  request.post(sample_data, function (error) {
    do_check_eq(error, null);

    do_check_eq(this.status, this.COMPLETED);
    do_check_true(this.response.success);
    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "");

    do_check_eq(handler.request.method, "POST");
    do_check_eq(handler.request.body, JSON.stringify(sample_data));
    do_check_eq(handler.request.getHeader("Content-Type"), "text/plain");

    server.stop(run_next_test);
  });
});




add_test(function test_put_override_content_type() {
  let handler = httpd_handler(200, "OK");
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest(server.baseURI + "/resource");
  request.setHeader("Content-Type", "application/lolcat");
  request.put("O HAI!!1!", function (error) {
    do_check_eq(error, null);

    do_check_eq(this.status, this.COMPLETED);
    do_check_true(this.response.success);
    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "");

    do_check_eq(handler.request.method, "PUT");
    do_check_eq(handler.request.body, "O HAI!!1!");
    do_check_eq(handler.request.getHeader("Content-Type"), "application/lolcat");

    server.stop(run_next_test);
  });
});




add_test(function test_post_override_content_type() {
  let handler = httpd_handler(200, "OK");
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest(server.baseURI + "/resource");
  request.setHeader("Content-Type", "application/lolcat");
  request.post("O HAI!!1!", function (error) {
    do_check_eq(error, null);

    do_check_eq(this.status, this.COMPLETED);
    do_check_true(this.response.success);
    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "");

    do_check_eq(handler.request.method, "POST");
    do_check_eq(handler.request.body, "O HAI!!1!");
    do_check_eq(handler.request.getHeader("Content-Type"), "application/lolcat");

    server.stop(run_next_test);
  });
});




add_test(function test_get_no_headers() {
  let handler = httpd_handler(200, "OK");
  let server = httpd_setup({"/resource": handler});

  let ignore_headers = ["host", "user-agent", "accept", "accept-language",
                        "accept-encoding", "accept-charset", "keep-alive",
                        "connection", "pragma", "cache-control",
                        "content-length"];

  new RESTRequest(server.baseURI + "/resource").get(function (error) {
    do_check_eq(error, null);

    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "");

    let server_headers = handler.request.headers;
    while (server_headers.hasMoreElements()) {
      let header = server_headers.getNext().toString();
      if (ignore_headers.indexOf(header) == -1) {
        do_throw("Got unexpected header!");
      }
    }

    server.stop(run_next_test);
  });
});




add_test(function test_changing_uri() {
  let handler = httpd_handler(200, "OK");
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest("http://localhost:1234/the-wrong-resource");
  request.uri = CommonUtils.makeURI(server.baseURI + "/resource");
  request.get(function (error) {
    do_check_eq(error, null);
    do_check_eq(this.response.status, 200);
    server.stop(run_next_test);
  });
});




add_test(function test_request_setHeader() {
  let handler = httpd_handler(200, "OK");
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest(server.baseURI + "/resource");

  request.setHeader("X-What-Is-Weave", "awesome");
  request.setHeader("X-WHAT-is-Weave", "more awesomer");
  request.setHeader("Another-Header", "Hello World");

  request.get(function (error) {
    do_check_eq(error, null);

    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "");

    do_check_eq(handler.request.getHeader("X-What-Is-Weave"), "more awesomer");
    do_check_eq(handler.request.getHeader("another-header"), "Hello World");

    server.stop(run_next_test);
  });
});




add_test(function test_response_headers() {
  function handler(request, response) {
    response.setHeader("X-What-Is-Weave", "awesome");
    response.setHeader("Another-Header", "Hello World");
    response.setStatusLine(request.httpVersion, 200, "OK");
  }
  let server = httpd_setup({"/resource": handler});
  let request = new RESTRequest(server.baseURI + "/resource");

  request.get(function (error) {
    do_check_eq(error, null);

    do_check_eq(this.response.status, 200);
    do_check_eq(this.response.body, "");

    do_check_eq(this.response.headers["x-what-is-weave"], "awesome");
    do_check_eq(this.response.headers["another-header"], "Hello World");

    server.stop(run_next_test);
  });
});





add_test(function test_connection_refused() {
  let request = new RESTRequest("http://localhost:1234/resource");
  request.onProgress = function onProgress() {
    do_throw("Shouldn't have called request.onProgress()!");
  };
  request.get(function (error) {
    do_check_eq(error.result, Cr.NS_ERROR_CONNECTION_REFUSED);
    do_check_eq(error.message, "NS_ERROR_CONNECTION_REFUSED");
    do_check_eq(this.status, this.COMPLETED);
    run_next_test();
  });
  do_check_eq(request.status, request.SENT);
});




add_test(function test_abort() {
  function handler() {
    do_throw("Shouldn't have gotten here!");
  }
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest(server.baseURI + "/resource");

  
  do_check_throws(function () {
    request.abort();
  });

  request.onProgress = request.onComplete = function () {
    do_throw("Shouldn't have gotten here!");
  };
  request.get();
  request.abort();

  
  do_check_throws(function () {
    request.abort();
  });

  do_check_eq(request.status, request.ABORTED);
  CommonUtils.nextTick(function () {
    server.stop(run_next_test);
  });
});





add_test(function test_timeout() {
  let server = new HttpServer();
  let server_connection;
  server._handler.handleResponse = function(connection) {
    
    
    
    
    server_connection = connection;
  };
  server.start();
  let identity = server.identity;
  let uri = identity.primaryScheme + "://" + identity.primaryHost + ":" +
            identity.primaryPort;

  let request = new RESTRequest(uri + "/resource");
  request.timeout = 0.1; 
  request.get(function (error) {
    do_check_eq(error.result, Cr.NS_ERROR_NET_TIMEOUT);
    do_check_eq(this.status, this.ABORTED);

    
    
    
    if (server_connection) {
      _("Closing connection.");
      server_connection.close();
    }

    _("Shutting down server.");
    server.stop(run_next_test);
  });
});




add_test(function test_exception_in_onProgress() {
  let handler = httpd_handler(200, "OK", "Foobar");
  let server = httpd_setup({"/resource": handler});

  let request = new RESTRequest(server.baseURI + "/resource");
  request.onProgress = function onProgress() {
    it.does.not.exist();
  };
  request.get(function onComplete(error) {
    do_check_eq(error, "ReferenceError: it is not defined");
    do_check_eq(this.status, this.ABORTED);

    server.stop(run_next_test);
  });
});

add_test(function test_new_channel() {
  _("Ensure a redirect to a new channel is handled properly.");

  function checkUA(metadata) {
    let ua = metadata.getHeader("User-Agent");
    _("User-Agent is " + ua);
    do_check_eq("foo bar", ua);
  }

  let redirectRequested = false;
  let redirectURL;
  function redirectHandler(metadata, response) {
    checkUA(metadata);
    redirectRequested = true;

    let body = "Redirecting";
    response.setStatusLine(metadata.httpVersion, 307, "TEMPORARY REDIRECT");
    response.setHeader("Location", redirectURL);
    response.bodyOutputStream.write(body, body.length);
  }

  let resourceRequested = false;
  function resourceHandler(metadata, response) {
    checkUA(metadata);
    resourceRequested = true;

    let body = "Test";
    response.setHeader("Content-Type", "text/plain");
    response.bodyOutputStream.write(body, body.length);
  }

  let server1 = httpd_setup({"/redirect": redirectHandler});
  let server2 = httpd_setup({"/resource": resourceHandler});
  redirectURL = server2.baseURI + "/resource";

  function advance() {
    server1.stop(function () {
      server2.stop(run_next_test);
    });
  }

  let request = new RESTRequest(server1.baseURI + "/redirect");
  request.setHeader("User-Agent", "foo bar");

  
  
  
  let protoMethod = request.shouldCopyOnRedirect;
  request.shouldCopyOnRedirect = function wrapped(o, n, f) {
    
    do_check_false(protoMethod.call(this, o, n, f));
    return true;
  };

  request.get(function onComplete(error) {
    let response = this.response;

    do_check_eq(200, response.status);
    do_check_eq("Test", response.body);
    do_check_true(redirectRequested);
    do_check_true(resourceRequested);

    advance();
  });
});

add_test(function test_not_sending_cookie() {
  function handler(metadata, response) {
    let body = "COOKIE!";
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.bodyOutputStream.write(body, body.length);
    do_check_false(metadata.hasHeader("Cookie"));
  }
  let server = httpd_setup({"/test": handler});

  let cookieSer = Cc["@mozilla.org/cookieService;1"]
                    .getService(Ci.nsICookieService);
  let uri = CommonUtils.makeURI(server.baseURI);
  cookieSer.setCookieString(uri, null, "test=test; path=/;", null);

  let res = new RESTRequest(server.baseURI + "/test");
  res.get(function (error) {
    do_check_null(error);
    do_check_true(this.response.success);
    do_check_eq("COOKIE!", this.response.body);
    server.stop(run_next_test);
  });
});


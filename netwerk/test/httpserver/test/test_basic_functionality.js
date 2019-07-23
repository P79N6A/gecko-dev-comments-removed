









































var tests =
  [
   new Test("http://localhost:4444/objHandler",
            null, start_objHandler, null),
   new Test("http://localhost:4444/functionHandler",
            null, start_functionHandler, null),
   new Test("http://localhost:4444/non-existent-path",
            null, start_non_existent_path, null),
  ];

function run_test()
{
  var srv = createServer();

  
  
  var dirServ = Cc["@mozilla.org/file/directory_service;1"]
                  .getService(Ci.nsIProperties);
  var path = dirServ.get("CurProcD", Ci.nsILocalFile);
  srv.registerDirectory("/", path);

  
  srv.registerPathHandler("/objHandler", objHandler);
  srv.registerPathHandler("/functionHandler", functionHandler);

  srv.start(4444);

  runHttpTests(tests, testComplete(srv));
}






function commonCheck(ch)
{
  do_check_true(ch.contentLength > -1);
  do_check_eq(ch.getResponseHeader("connection"), "close");
  do_check_false(ch.isNoStoreResponse());
}

function start_objHandler(ch, cx)
{
  commonCheck(ch);

  do_check_eq(ch.responseStatus, 200);
  do_check_true(ch.requestSucceeded);
  do_check_eq(ch.getResponseHeader("content-type"), "text/plain");
  do_check_eq(ch.responseStatusText, "OK");

  var reqMin = {}, reqMaj = {}, respMin = {}, respMaj = {};
  ch.getRequestVersion(reqMaj, reqMin);
  ch.getResponseVersion(respMaj, respMin);
  do_check_true(reqMaj.value == respMaj.value &&
                reqMin.value == respMin.value);
}

function start_functionHandler(ch, cx)
{
  commonCheck(ch);

  do_check_eq(ch.responseStatus, 404);
  do_check_false(ch.requestSucceeded);
  do_check_eq(ch.getResponseHeader("foopy"), "quux-baz");
  do_check_eq(ch.responseStatusText, "Page Not Found");

  var reqMin = {}, reqMaj = {}, respMin = {}, respMaj = {};
  ch.getRequestVersion(reqMaj, reqMin);
  ch.getResponseVersion(respMaj, respMin);
  do_check_true(reqMaj.value == 1 && reqMin.value == 1);
  do_check_true(respMaj.value == 1 && respMin.value == 1);
}

function start_non_existent_path(ch, cx)
{
  commonCheck(ch);

  do_check_eq(ch.responseStatus, 404);
  do_check_false(ch.requestSucceeded);
}





var objHandler =
  {
    handle: function(metadata, response)
    {
      response.setStatusLine(metadata.httpVersion, 200, "OK");
      response.setHeader("Content-Type", "text/plain", false);

      var body = "Request (slightly reformatted):\n\n";
      body += metadata.method + " " + metadata.path;

      do_check_eq(metadata.port, 4444);

      if (metadata.queryString)
        body +=  "?" + metadata.queryString;

      body += " HTTP/" + metadata.httpVersion + "\n";
        
      var headEnum = metadata.headers;
      while (headEnum.hasMoreElements())
      {
        var fieldName = headEnum.getNext()
                                .QueryInterface(Ci.nsISupportsString)
                                .data;
        body += fieldName + ": " + metadata.getHeader(fieldName) + "\n";
      }

      response.bodyOutputStream.write(body, body.length);
    },
    QueryInterface: function(id)
    {
      if (id.equals(Ci.nsISupports) || id.equals(Ci.nsIHttpRequestHandler))
        return this;
      throw Cr.NS_ERROR_NOINTERFACE;
    }
  };


function functionHandler(metadata, response)
{
  response.setStatusLine("1.1", 404, "Page Not Found");
  response.setHeader("foopy", "quux-baz", false);

  do_check_eq(metadata.port, 4444);
  do_check_eq(metadata.host, "localhost");
  do_check_eq(metadata.path.charAt(0), "/");

  var body = "this is text\n";
  response.bodyOutputStream.write(body, body.length);
}

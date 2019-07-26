







var srv, serverBasePath;

function run_test()
{
  srv = createServer();
  serverBasePath = do_get_profile();
  srv.registerDirectory("/", serverBasePath);
  srv.setIndexHandler(myIndexHandler);
  srv.start(-1);

  runHttpTests(tests, testComplete(srv));
}

XPCOMUtils.defineLazyGetter(this, "URL", function() {
  return "http://localhost:" + srv.identity.primaryPort + "/";
});

XPCOMUtils.defineLazyGetter(this, "tests", function() {
  return [
    new Test(URL, init, startCustomIndexHandler, stopCustomIndexHandler),
    new Test(URL, init, startDefaultIndexHandler, stopDefaultIndexHandler)
  ];
});

function init(ch)
{
  ch.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE; 
}
function startCustomIndexHandler(ch, cx)
{
  do_check_eq(ch.getResponseHeader("Content-Length"), "10");
  srv.setIndexHandler(null);
}
function stopCustomIndexHandler(ch, cx, status, data)
{
  do_check_true(Components.isSuccessCode(status));
  do_check_eq(String.fromCharCode.apply(null, data), "directory!");
}

function startDefaultIndexHandler(ch, cx)
{
  do_check_eq(ch.responseStatus, 200);
}
function stopDefaultIndexHandler(ch, cx, status, data)
{
  do_check_true(Components.isSuccessCode(status));
}



function myIndexHandler(metadata, response)
{
  var dir = metadata.getProperty("directory");
  do_check_true(dir != null);
  do_check_true(dir instanceof Ci.nsIFile);
  do_check_true(dir.equals(serverBasePath));

  response.write("directory!");
}

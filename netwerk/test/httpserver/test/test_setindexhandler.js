







































var srv, serverBasePath;

function run_test()
{
  srv = createServer();
  serverBasePath = do_get_cwd();
  srv.registerDirectory("/", serverBasePath);
  srv.setIndexHandler(myIndexHandler);
  srv.start(4444);

  runHttpTests(tests, function() { srv.stop(); });
}


var tests = [];
var test;

test = new Test("http://localhost:4444/",
                init, startCustomIndexHandler, stopCustomIndexHandler);
tests.push(test);
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

test = new Test("http://localhost:4444/",
                init, startDefaultIndexHandler, stopDefaultIndexHandler);
tests.push(test);
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

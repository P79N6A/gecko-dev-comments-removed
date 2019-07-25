







const BASE = "http://localhost:4444";

var tests = [];
var test;

function nocache(ch)
{
  ch.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE; 
}

function notFound(ch)
{
  do_check_eq(ch.responseStatus, 404);
  do_check_false(ch.requestSucceeded);
}

function makeCheckOverride(magic)
{
  return (function checkOverride(ch)
  {
    do_check_eq(ch.responseStatus, 200);
    do_check_eq(ch.responseStatusText, "OK");
    do_check_true(ch.requestSucceeded);
    do_check_eq(ch.getResponseHeader("Override-Succeeded"), magic);
  });
}





function prefixHandler(channel)
{
  nocache(channel);
  srv.registerPrefixHandler("/prefix/", makeOverride("prefix"));
}

test = new Test(BASE + "/prefix/dummy", prefixHandler, null,
                makeCheckOverride("prefix"));
tests.push(test);





function pathHandler(channel)
{
  nocache(channel);
  srv.registerPathHandler("/prefix/dummy", makeOverride("path"));
}
test = new Test(BASE + "/prefix/dummy", pathHandler, null,
                makeCheckOverride("path"));
tests.push(test);





function longerPrefixHandler(channel)
{
  nocache(channel);
  srv.registerPrefixHandler("/prefix/subpath/", makeOverride("subpath"));
}
test = new Test(BASE + "/prefix/subpath/dummy", longerPrefixHandler, null,
                makeCheckOverride("subpath"));
tests.push(test);





function removeHandlers(channel)
{
  nocache(channel);
  srv.registerPrefixHandler("/prefix/", null);
  srv.registerPathHandler("/prefix/dummy", null);
}
test = new Test(BASE + "/prefix/dummy", removeHandlers, null, notFound);
tests.push(test);





function newPrefixHandler(channel)
{
  nocache(channel);
  srv.registerPrefixHandler("/prefix/", makeOverride("prefix"));
}
test = new Test(BASE + "/prefix/subpath/dummy", newPrefixHandler, null,
                makeCheckOverride("subpath"));
tests.push(test);

var srv;
var serverBasePath;
var testsDirectory;

function run_test()
{
  testsDirectory = do_get_cwd();

  srv = createServer();
  srv.start(4444);

  runHttpTests(tests, testComplete(srv));
}




function makeOverride(magic)
{
  return (function override(metadata, response)
  {
    response.setStatusLine("1.1", 200, "OK");
    response.setHeader("Override-Succeeded", magic, false);

    var body = "success!";
    response.bodyOutputStream.write(body, body.length);
  });
}

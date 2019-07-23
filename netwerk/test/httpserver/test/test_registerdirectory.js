







































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

function checkOverride(ch)
{
  do_check_eq(ch.responseStatus, 200);
  do_check_eq(ch.responseStatusText, "OK");
  do_check_true(ch.requestSucceeded);
  do_check_eq(ch.getResponseHeader("Override-Succeeded"), "yes");
}

function check200(ch)
{
  do_check_eq(ch.responseStatus, 200);
  do_check_eq(ch.responseStatusText, "OK");
}

function checkFile(ch, cx, status, data)
{
  do_check_eq(ch.responseStatus, 200);
  do_check_true(ch.requestSucceeded);

  var actualFile = serverBasePath.clone();
  actualFile.append("test_registerdirectory.js");
  do_check_eq(ch.getResponseHeader("Content-Length"),
              actualFile.fileSize.toString());
  do_check_eq(data.map(function(v) String.fromCharCode(v)).join(""),
              fileContents(actualFile));
}






test = new Test(BASE + "/test_registerdirectory.js",
                nocache, notFound, null),
tests.push(test);






test = new Test(BASE + "/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);
                  serverBasePath = testsDirectory.clone();
                  srv.registerDirectory("/", serverBasePath);
                },
                null,
                checkFile);
tests.push(test);






test = new Test(BASE + "/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);
                  serverBasePath = null;
                  srv.registerDirectory("/", serverBasePath);
                },
                notFound,
                null);
tests.push(test);






test = new Test(BASE + "/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);
                  srv.registerPathHandler("/test_registerdirectory.js",
                                          override_test_registerdirectory);
                },
                checkOverride,
                null);
tests.push(test);






test = new Test(BASE + "/test_registerdirectory.js",
                function init_registerDirectory6(ch)
                {
                  nocache(ch);
                  srv.registerPathHandler("/test_registerdirectory.js", null);
                },
                notFound,
                null);
tests.push(test);






test = new Test(BASE + "/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);

                  
                  serverBasePath = testsDirectory.clone();
                  srv.registerDirectory("/", serverBasePath);
                },
                null,
                checkFile);
tests.push(test);






test = new Test(BASE + "/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);
                  srv.registerPathHandler("/test_registerdirectory.js",
                                          override_test_registerdirectory);
                },
                checkOverride,
                null);
tests.push(test);






test = new Test(BASE + "/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);
                  serverBasePath = null;
                  srv.registerDirectory("/", serverBasePath);
                },
                checkOverride,
                null);
tests.push(test);






test = new Test(BASE + "/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);
                  srv.registerPathHandler("/test_registerdirectory.js", null);
                },
                notFound,
                null);
tests.push(test);






test = new Test(BASE + "/foo/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);
                  serverBasePath = testsDirectory.clone();
                  srv.registerDirectory("/foo/", serverBasePath);
                },
                check200,
                null);
tests.push(test);






test = new Test(BASE + "/foo/test_registerdirectory.js/test_registerdirectory.js",
                nocache,
                notFound,
                null);
tests.push(test);






test = new Test(BASE + "/foo/test_registerdirectory.js/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);
                  srv.registerDirectory("/foo/test_registerdirectory.js/",
                                        serverBasePath);
                },
                null,
                checkFile);
tests.push(test);






test = new Test(BASE + "/foo/test_registerdirectory.js",
                nocache, null, checkFile);
tests.push(test);






test = new Test(BASE + "/foo/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);
                  srv.registerDirectory("/foo/", null);
                },
                notFound,
                null);
tests.push(test);






test = new Test(BASE + "/foo/test_registerdirectory.js/test_registerdirectory.js",
                nocache, null, checkFile);
tests.push(test);






test = new Test(BASE + "/foo/test_registerdirectory.js/test_registerdirectory.js",
                function(ch)
                {
                  nocache(ch);
                  srv.registerDirectory("/foo/test_registerdirectory.js/", null);
                },
                notFound,
                null);
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





function override_test_registerdirectory(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
  response.setHeader("Override-Succeeded", "yes", false);

  var body = "success!";
  response.bodyOutputStream.write(body, body.length);
}

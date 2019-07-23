







































const BASE = "http://localhost:4444";

var sjs = do_get_file("data/sjs/cgi.sjs");
var srv;
var test;
var tests = [];






function bytesToString(bytes)
{
  return bytes.map(function(v) { return String.fromCharCode(v); }).join("");
}

function skipCache(ch)
{
  ch.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
}











function setupTests(throwing)
{
  const TEST_URL = BASE + "/cgi.sjs" + (throwing ? "?throw" : "");

  

  function setupFile(ch)
  {
    srv.registerFile("/cgi.sjs", sjs);
    skipCache(ch);
  }

  function verifyRawText(channel, cx, status, bytes)
  {
    dumpn(channel.originalURI.spec);
    do_check_eq(bytesToString(bytes), fileContents(sjs));
  }

  test = new Test(TEST_URL, setupFile, null, verifyRawText);
  tests.push(test);


  

  function addTypeMapping(ch)
  {
    srv.registerContentType("sjs", "sjs");
    skipCache(ch);
  }

  function checkType(ch, cx)
  {
    if (throwing)
    {
      do_check_false(ch.requestSucceeded);
      do_check_eq(ch.responseStatus, 500);
    }
    else
    {
      do_check_eq(ch.contentType, "text/plain");
    }
  }

  function checkContents(ch, cx, status, data)
  {
    if (!throwing)
      do_check_eq("PASS", bytesToString(data));
  }

  test = new Test(TEST_URL, addTypeMapping, checkType, checkContents);
  tests.push(test);


  

  function setupDirectoryAndRemoveType(ch)
  {
    dumpn("removing type mapping");
    srv.registerContentType("sjs", null);
    srv.registerFile("/cgi.sjs", null);
    srv.registerDirectory("/", sjs.parent);
    skipCache(ch);
  }

  test = new Test(TEST_URL, setupDirectoryAndRemoveType, null, verifyRawText);
  tests.push(test);


  
  
  function contentAndCleanup(ch, cx, status, data)
  {
    checkContents(ch, cx, status, data);

    
    srv.registerDirectory("/", null);
    srv.registerContentType("sjs", null);
  }

  test = new Test(TEST_URL, addTypeMapping, checkType, contentAndCleanup);
  tests.push(test);

  
  
  
}






setupTests(true);
setupTests(false);





function init(ch)
{
  
  srv.registerDirectory("/", sjs.parent);
  srv.registerContentType("sjs", "sjs");
  skipCache(ch);
}

function checkNotSJS(ch, cx, status, data)
{
  do_check_neq("FAIL", bytesToString(data));
}

test = new Test(BASE + "/sjs", init, null, checkNotSJS);
tests.push(test);




function rangeInit(expectedRangeHeader)
{
  return function setupRangeRequest(ch)
  {
    ch.setRequestHeader("Range", expectedRangeHeader, false);
  };
}

function checkRangeResult(ch, cx)
{
  try
  {
    var val = ch.getResponseHeader("Content-Range");
  }
  catch (e) {  }
  if (val !== undefined)
  {
    do_throw("should not have gotten a Content-Range header, but got one " +
             "with this value: " + val);
  }
  do_check_eq(200, ch.responseStatus);
  do_check_eq("OK", ch.responseStatusText);
}

test = new Test(BASE + "/range-checker.sjs",
                rangeInit("not-a-bytes-equals-specifier"),
                checkRangeResult, null);
tests.push(test);
test = new Test(BASE + "/range-checker.sjs",
                rangeInit("bytes=-"),
                checkRangeResult, null);
tests.push(test);
test = new Test(BASE + "/range-checker.sjs",
                rangeInit("bytes=1000000-"),
                checkRangeResult, null);
tests.push(test);
test = new Test(BASE + "/range-checker.sjs",
                rangeInit("bytes=1-4"),
                checkRangeResult, null);
tests.push(test);
test = new Test(BASE + "/range-checker.sjs",
                rangeInit("bytes=-4"),
                checkRangeResult, null);
tests.push(test);





function setupFileMapping(ch)
{
  srv.registerFile("/script.html", sjs);
}

function onStart(ch, cx)
{
  do_check_eq(ch.contentType, "text/plain");
}

function onStop(ch, cx, status, data)
{
  do_check_eq("PASS", bytesToString(data));
}

test = new Test(BASE + "/script.html", setupFileMapping, onStart, onStop);
tests.push(test);






function run_test()
{
  srv = createServer();

  
  try
  {
    srv.registerContentType("foo", "bar\nbaz");
    throw "this server throws on content-types which aren't field-values";
  }
  catch (e)
  {
    isException(e, Cr.NS_ERROR_INVALID_ARG);
  }


  
  
  
  

  srv.start(4444);
  runHttpTests(tests, testComplete(srv));
}

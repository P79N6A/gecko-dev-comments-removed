








var srv;

XPCOMUtils.defineLazyGetter(this, "tests", function() {
  return [
    new Test("http://localhost:" + srv.identity.primaryPort + "/empty-body-unwritten",
             null, ensureEmpty, null),
    new Test("http://localhost:" + srv.identity.primaryPort + "/empty-body-written",
             null, ensureEmpty, null),
  ];
});

function run_test()
{
  srv = createServer();

  
  srv.registerPathHandler("/empty-body-unwritten", emptyBodyUnwritten);
  srv.registerPathHandler("/empty-body-written", emptyBodyWritten);

  srv.start(-1);

  runHttpTests(tests, testComplete(srv));
}



function ensureEmpty(ch, cx)
{
  do_check_true(ch.contentLength == 0);
}




function emptyBodyUnwritten(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
}


function emptyBodyWritten(metadata, response)
{
  response.setStatusLine("1.1", 200, "OK");
  var body = "";
  response.bodyOutputStream.write(body, body.length);
}

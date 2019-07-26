







XPCOMUtils.defineLazyGetter(this, "BASE", function() {
  return "http://localhost:" + srv.identity.primaryPort;
});

var file = do_get_file("test_registerfile.js");

function onStart(ch, cx)
{
  do_check_eq(ch.responseStatus, 200);
}

function onStop(ch, cx, status, data)
{
  
  do_check_eq(data.length, file.fileSize);
}

XPCOMUtils.defineLazyGetter(this, "test", function() {
  return new Test(BASE + "/foo", null, onStart, onStop);
});

var srv;

function run_test()
{
  srv = createServer();

  try
  {
    srv.registerFile("/foo", do_get_profile());
    throw "registerFile succeeded!";
  }
  catch (e)
  {
    isException(e, Cr.NS_ERROR_INVALID_ARG);
  }

  srv.registerFile("/foo", file);
  srv.start(-1);

  runHttpTests([test], testComplete(srv));
}

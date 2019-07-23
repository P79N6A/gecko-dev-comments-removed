







































const BASE = "http://localhost:4444";

function isException(e, code)
{
  if (e !== code && e.result !== code)
    do_throw("unexpected error: " + e);
}

var file = do_get_file("test_registerfile.js");

function onStart(ch, cx)
{
  do_check_eq(ch.responseStatus, 200);
}

function onStop(ch, cx, status, data)
{
  
  do_check_eq(data.length, file.fileSize);
}

var test = new Test(BASE + "/foo", null, onStart, onStop);

function run_test()
{
  var srv = createServer();

  try
  {
    srv.registerFile("/foo", do_get_cwd());
    throw "registerFile succeeded!";
  }
  catch (e)
  {
    isException(e, Cr.NS_ERROR_INVALID_ARG);
  }

  srv.registerFile("/foo", file);
  srv.start(4444);

  runHttpTests([test], function() { srv.stop(); });
}

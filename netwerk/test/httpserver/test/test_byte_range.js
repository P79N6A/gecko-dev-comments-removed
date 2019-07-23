






































const PREFIX = "http://localhost:4444";

var tests =
  [
   new  Test(PREFIX + "/normal-file.txt",
             init_byterange, start_byterange, stop_byterange),
   new  Test(PREFIX + "/normal-file.txt",
             null, start_normal, stop_normal)
   ];


function run_test()
{
  var srv = createServer();
  var dir = do_get_file("netwerk/test/httpserver/test/data/name-scheme/");
  srv.registerDirectory("/", dir);

  srv.start(4444);

  runHttpTests(tests, function() { srv.stop(); });
}

function start_normal(ch, cx)
{
  do_check_eq(ch.responseStatus, 200);
  do_check_eq(ch.getResponseHeader("Content-Type"), "text/plain");
}

function stop_normal(ch, cx, status, data)
{
  do_check_eq(data[0], 84);
  do_check_eq(data[20], 10);
  do_check_eq(data.length, 21);
}

function init_byterange(ch)
{
  ch.setRequestHeader("Range", "bytes=10-", false);
}


function start_byterange(ch, cx)
{
  do_check_eq(ch.responseStatus, 206);
  do_check_eq(ch.getResponseHeader("Content-Type"), "text/plain");
}

function stop_byterange(ch, cx, status, data)
{
  do_check_eq(data[0], 100);
  do_check_eq(data[10], 10);
  do_check_eq(data.length, 11);
}


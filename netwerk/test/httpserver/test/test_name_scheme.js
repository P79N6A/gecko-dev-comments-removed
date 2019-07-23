









































const PREFIX = "http://localhost:4444";

var tests =
  [
   new Test(PREFIX + "/bar.html^",
            null, start_bar_html_, null),
   new Test(PREFIX + "/foo.html^",
            null, start_foo_html_, null),
   new Test(PREFIX + "/normal-file.txt",
            null, start_normal_file_txt, null),
   new Test(PREFIX + "/folder^/file.txt",
            null, start_folder__file_txt, null),

   new Test(PREFIX + "/foo/bar.html^",
            null, start_bar_html_, null),
   new Test(PREFIX + "/foo/foo.html^",
            null, start_foo_html_, null),
   new Test(PREFIX + "/foo/normal-file.txt",
            null, start_normal_file_txt, null),
   new Test(PREFIX + "/foo/folder^/file.txt",
            null, start_folder__file_txt, null),

   new Test(PREFIX + "/end-caret^/bar.html^",
            null, start_bar_html_, null),
   new Test(PREFIX + "/end-caret^/foo.html^",
            null, start_foo_html_, null),
   new Test(PREFIX + "/end-caret^/normal-file.txt",
            null, start_normal_file_txt, null),
   new Test(PREFIX + "/end-caret^/folder^/file.txt",
            null, start_folder__file_txt, null)
  ];


function run_test()
{
  var srv = createServer();

  
  
  var nameDir = do_get_file("netwerk/test/httpserver/test/data/name-scheme/");
  srv.registerDirectory("/", nameDir);
  srv.registerDirectory("/foo/", nameDir);
  srv.registerDirectory("/end-caret^/", nameDir);

  srv.start(4444);

  runHttpTests(tests, function() { srv.stop(); });
}




function start_bar_html_(ch, cx)
{
  do_check_eq(ch.responseStatus, 200);

  do_check_eq(ch.getResponseHeader("Content-Type"), "text/html");
}

function start_foo_html_(ch, cx)
{
  do_check_eq(ch.responseStatus, 404);
}

function start_normal_file_txt(ch, cx)
{
  do_check_eq(ch.responseStatus, 200);
  do_check_eq(ch.getResponseHeader("Content-Type"), "text/plain");
}

function start_folder__file_txt(ch, cx)
{
  do_check_eq(ch.responseStatus, 200);
  do_check_eq(ch.getResponseHeader("Content-Type"), "text/plain");
}







function run_test() {
  Components.utils.import("resource://testing-common/httpd.js");

  let server = new HttpServer();
  server.start(-1);

  do_test_pending();

  server.stop(do_test_finished);
}







Components.utils.import("resource://testing-common/httpd.js");

function run_test() {
  var httpserver = new HttpServer();
  do_check_neq(httpserver, null);
  do_check_neq(httpserver.QueryInterface(Components.interfaces.nsIHttpServer), null);
}

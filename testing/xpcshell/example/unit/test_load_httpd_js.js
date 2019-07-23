






































do_load_httpd_js();

function run_test() {
  var httpserver = new nsHttpServer();
  do_check_neq(httpserver, null);
  do_check_neq(httpserver.QueryInterface(Components.interfaces.nsIHttpServer), null);
}

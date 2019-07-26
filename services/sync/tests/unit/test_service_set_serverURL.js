


Cu.import("resource://services-sync/service.js");

function run_test() {
  Service.serverURL = "http://example.com/sync";
  do_check_eq(Service.serverURL, "http://example.com/sync/");

  Service.serverURL = "http://example.com/sync/";
  do_check_eq(Service.serverURL, "http://example.com/sync/");
}


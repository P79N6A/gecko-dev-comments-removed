


Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/rest.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://testing-common/services/sync/utils.js");

function run_test() {
  Log.repository.getLogger("Sync.RESTRequest").level = Log.Level.Trace;
  initTestLogging();

  run_next_test();
}

add_test(function test_user_agent_desktop() {
  let handler = httpd_handler(200, "OK");
  let server = httpd_setup({"/resource": handler});

  let expectedUA = Services.appinfo.name + "/" + Services.appinfo.version +
                   " FxSync/" + WEAVE_VERSION + "." +
                   Services.appinfo.appBuildID + ".desktop";

  let request = new SyncStorageRequest(server.baseURI + "/resource");
  request.onComplete = function onComplete(error) {
    do_check_eq(error, null);
    do_check_eq(this.response.status, 200);
    do_check_eq(handler.request.getHeader("User-Agent"), expectedUA);
    server.stop(run_next_test);
  };
  do_check_eq(request.get(), request);
});

add_test(function test_user_agent_mobile() {
  let handler = httpd_handler(200, "OK");
  let server = httpd_setup({"/resource": handler});

  Svc.Prefs.set("client.type", "mobile");
  let expectedUA = Services.appinfo.name + "/" + Services.appinfo.version +
                   " FxSync/" + WEAVE_VERSION + "." +
                   Services.appinfo.appBuildID + ".mobile";

  let request = new SyncStorageRequest(server.baseURI + "/resource");
  request.get(function (error) {
    do_check_eq(error, null);
    do_check_eq(this.response.status, 200);
    do_check_eq(handler.request.getHeader("User-Agent"), expectedUA);
    Svc.Prefs.resetBranch("");
    server.stop(run_next_test);
  });
});

add_test(function test_auth() {
  let handler = httpd_handler(200, "OK");
  let server = httpd_setup({"/resource": handler});

  setBasicCredentials("johndoe", "ilovejane", "XXXXXXXXX");

  let request = Service.getStorageRequest(server.baseURI + "/resource");
  request.get(function (error) {
    do_check_eq(error, null);
    do_check_eq(this.response.status, 200);
    do_check_true(basic_auth_matches(handler.request, "johndoe", "ilovejane"));

    Svc.Prefs.reset("");

    server.stop(run_next_test);
  });
});




add_test(function test_weave_timestamp() {
  const TIMESTAMP = 1274380461;
  function handler(request, response) {
    response.setHeader("X-Weave-Timestamp", "" + TIMESTAMP, false);
    response.setStatusLine(request.httpVersion, 200, "OK");
  }
  let server = httpd_setup({"/resource": handler});

  do_check_eq(SyncStorageRequest.serverTime, undefined);
  let request = new SyncStorageRequest(server.baseURI + "/resource");
  request.get(function (error) {
    do_check_eq(error, null);
    do_check_eq(this.response.status, 200);
    do_check_eq(SyncStorageRequest.serverTime, TIMESTAMP);
    delete SyncStorageRequest.serverTime;
    server.stop(run_next_test);
  });
});




add_test(function test_weave_backoff() {
  function handler(request, response) {
    response.setHeader("X-Weave-Backoff", '600', false);
    response.setStatusLine(request.httpVersion, 200, "OK");
  }
  let server = httpd_setup({"/resource": handler});

  let backoffInterval;
  Svc.Obs.add("weave:service:backoff:interval", function onBackoff(subject) {
    Svc.Obs.remove("weave:service:backoff:interval", onBackoff);
    backoffInterval = subject;
  });

  let request = new SyncStorageRequest(server.baseURI + "/resource");
  request.get(function (error) {
    do_check_eq(error, null);
    do_check_eq(this.response.status, 200);
    do_check_eq(backoffInterval, 600);
    server.stop(run_next_test);
  });
});




add_test(function test_weave_quota_notice() {
  function handler(request, response) {
    response.setHeader("X-Weave-Quota-Remaining", '1048576', false);
    response.setStatusLine(request.httpVersion, 200, "OK");
  }
  let server = httpd_setup({"/resource": handler});

  let quotaValue;
  Svc.Obs.add("weave:service:quota:remaining", function onQuota(subject) {
    Svc.Obs.remove("weave:service:quota:remaining", onQuota);
    quotaValue = subject;
  });

  let request = new SyncStorageRequest(server.baseURI + "/resource");
  request.get(function (error) {
    do_check_eq(error, null);
    do_check_eq(this.response.status, 200);
    do_check_eq(quotaValue, 1048576);
    server.stop(run_next_test);
  });
});




add_test(function test_weave_quota_error() {
  function handler(request, response) {
    response.setHeader("X-Weave-Quota-Remaining", '1048576', false);
    response.setStatusLine(request.httpVersion, 400, "Bad Request");
  }
  let server = httpd_setup({"/resource": handler});

  let quotaValue;
  function onQuota(subject) {
    quotaValue = subject;
  }
  Svc.Obs.add("weave:service:quota:remaining", onQuota);

  let request = new SyncStorageRequest(server.baseURI + "/resource");
  request.get(function (error) {
    do_check_eq(error, null);
    do_check_eq(this.response.status, 400);
    do_check_eq(quotaValue, undefined);
    Svc.Obs.remove("weave:service:quota:remaining", onQuota);
    server.stop(run_next_test);
  });
});

add_test(function test_abort() {
  function handler(request, response) {
    response.setHeader("X-Weave-Timestamp", "" + TIMESTAMP, false);
    response.setHeader("X-Weave-Quota-Remaining", '1048576', false);
    response.setHeader("X-Weave-Backoff", '600', false);
    response.setStatusLine(request.httpVersion, 200, "OK");
  }
  let server = httpd_setup({"/resource": handler});

  let request = new SyncStorageRequest(server.baseURI + "/resource");

  
  do_check_throws(function () {
    request.abort();
  });

  function throwy() {
    do_throw("Shouldn't have gotten here!");
  }

  Svc.Obs.add("weave:service:backoff:interval", throwy);
  Svc.Obs.add("weave:service:quota:remaining", throwy);
  request.onProgress = request.onComplete = throwy;

  request.get();
  request.abort();
  do_check_eq(request.status, request.ABORTED);

  
  do_check_throws(function () {
    request.abort();
  });

  Utils.nextTick(function () {
    
    do_check_eq(SyncStorageRequest.serverTime, undefined);

    Svc.Obs.remove("weave:service:backoff:interval", throwy);
    Svc.Obs.remove("weave:service:quota:remaining", throwy);

    server.stop(run_next_test);
  });
});

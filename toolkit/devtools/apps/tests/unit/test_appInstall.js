


Cu.import("resource://testing-common/httpd.js");




XPCOMUtils.defineLazyGetter(this, "appsService", function() {
  return Cc["@mozilla.org/AppsService;1"]
           .getService(Ci.nsIAppsService);
});

const SERVER_PORT = 4444;
const HTTP_BASE = "http://localhost:" + SERVER_PORT + "/";

const APP_ID = "actor-test";
const APP_BASE = "app://" + APP_ID + "/";

const APP_MANIFEST = APP_BASE + "manifest.webapp";

const REDIRECTED_URL = HTTP_BASE + "redirection-source.html";

const REMOTE_REDIRECTION_URL = HTTP_BASE + "redirection-target.html";

const LOCAL_REDIRECTION_URL = APP_BASE + "redirected.html";

add_test(function testInstallApp() {
  installTestApp("app-redirect.zip", APP_ID, run_next_test);
});

add_test(function testLaunchApp() {
  let server = new HttpServer();

  
  
  
  server.registerPathHandler("/redirection-source.html", function handle(request, response) {
    response.setStatusLine(request.httpVersion, 301, "Moved Permanently");
    response.setHeader("Location", REMOTE_REDIRECTION_URL, false);

    
    do_timeout(0, function () {
      do_check_eq(d.document.documentURIObject.spec, LOCAL_REDIRECTION_URL);
      server.stop(run_next_test);
    });
  });
  server.registerPathHandler("/redirection-target.html", function handle(request, response) {
    do_throw("We shouldn't receive any request to the remote redirection");
  });
  server.start(SERVER_PORT)

  
  let d = Cc["@mozilla.org/docshell;1"].createInstance(Ci.nsIDocShell);
  d.QueryInterface(Ci.nsIWebNavigation);
  d.QueryInterface(Ci.nsIWebProgress);

  let localAppId = appsService.getAppLocalIdByManifestURL(APP_MANIFEST);
  d.setIsApp(localAppId);
  do_check_true(d.isApp);
  do_check_eq(d.appId, localAppId);

  d.loadURI(REDIRECTED_URL, Ci.nsIWebNavigation.LOAD_FLAGS_NONE, null, null, null);
});

function run_test() {
  setup();

  run_next_test();
}


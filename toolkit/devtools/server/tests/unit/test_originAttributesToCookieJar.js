


Cu.import("resource://gre/modules/Services.jsm");
var ssm = Services.scriptSecurityManager;

function run_test() {
  const appId = 12;
  var browserAttrs = {appId: appId, inBrowser: true};

  
  
  var cookieJar_1 = ChromeUtils.originAttributesToCookieJar(browserAttrs);
  var dummy = Services.io.newURI("http://example.com", null, null);
  var cookieJar_2 = ssm.createCodebasePrincipal(dummy, browserAttrs).cookieJar;
  do_check_eq(cookieJar_1, cookieJar_2);

  
  var appAttrs = {appId: appId, inBrowser: false};
  var cookieJar_3 = ChromeUtils.originAttributesToCookieJar(appAttrs);
  do_check_neq(cookieJar_1, cookieJar_3);

  
  var cookieJar_4 = ChromeUtils.originAttributesToCookieJar();
  do_check_eq(cookieJar_4, "");
}

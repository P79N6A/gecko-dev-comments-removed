


Components.utils.import("resource://gre/modules/NetUtil.jsm");

function run_test() {
  var cs = Cc["@mozilla.org/cookieService;1"].getService(Ci.nsICookieService);
  var cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  var expiry = (Date.now() + 1000) * 1000;

  
  
  cm.add("a", "/", "foo", "bar", false, false, true, expiry);
  do_check_eq(cm.countCookiesFromHost("a"), 1);
  do_check_eq(cs.getCookieString(NetUtil.newURI("http://a"), null), "foo=bar");
}

Cu.import("resource://gre/modules/Services.jsm");

function makeURI(str) {
    return Components.classes["@mozilla.org/network/io-service;1"]
                     .getService(Components.interfaces.nsIIOService)
                     .newURI(str, null, null);
}

function run_test() {
    
    Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);
    var serv =   Components.classes["@mozilla.org/cookieService;1"]
                           .getService(Components.interfaces.nsICookieService);
    var uri = makeURI("http://example.com/");
    
    serv.setCookieString(uri, null, "test=test; path=/; domain=example.com; expires=Sun, 31-Dec-1899 16:00:00 GMT;", null);
    do_check_eq(serv.getCookieString(uri, null), null);
    
    serv.setCookieString(uri, null, "test2=test2; path=/; domain=example.com;", null);
    do_check_eq(serv.getCookieString(uri, null), "test2=test2");
}

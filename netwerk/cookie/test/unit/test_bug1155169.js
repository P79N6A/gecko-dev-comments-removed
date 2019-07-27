const {utils: Cu, interfaces: Ci, classes: Cc} = Components;

Cu.import("resource://gre/modules/Services.jsm");

const URI = Services.io.newURI("http://example.org/", null, null);

const cs = Cc["@mozilla.org/cookieService;1"]
             .getService(Ci.nsICookieService);

function run_test() {
  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

  
  Services.cookies.removeAll();

  
  setCookie("foo=bar", {
    type: "added", isSession: true, isSecure: false, isHttpOnly: false
  });

  
  setCookie("foo=bar; HttpOnly", {
    type: "changed", isSession: true, isSecure: false, isHttpOnly: true
  });

  
  setCookie("foo=bar; Secure", {
    type: "changed", isSession: true, isSecure: true, isHttpOnly: false
  });

  
  let expiry = new Date();
  expiry.setUTCFullYear(expiry.getUTCFullYear() + 2);
  setCookie(`foo=bar; Expires=${expiry.toGMTString()}`, {
    type: "changed", isSession: false, isSecure: false, isHttpOnly: false
  });

  
  setCookie("foo=bar", {
    type: "changed", isSession: true, isSecure: false, isHttpOnly: false
  });
}

function setCookie(value, expected) {
  function setCookieInternal(value, expected = null) {
    function observer(subject, topic, data) {
      if (!expected) {
        do_throw("no notification expected");
        return;
      }

      
      do_check_eq(data, expected.type);

      
      let cookie = subject.QueryInterface(Ci.nsICookie2);
      do_check_eq(cookie.isSession, expected.isSession);
      do_check_eq(cookie.isSecure, expected.isSecure);
      do_check_eq(cookie.isHttpOnly, expected.isHttpOnly);
    }

    Services.obs.addObserver(observer, "cookie-changed", false);
    cs.setCookieStringFromHttp(URI, null, null, value, null, null);
    Services.obs.removeObserver(observer, "cookie-changed");
  }

  
  setCookieInternal(value, expected);

  
  setCookieInternal(value);
}

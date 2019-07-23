



function test() {
  waitForExplicitFinish();

  var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch);
  prefs.setIntPref("network.cookie.cookieBehavior", 1);

  var o = new obs();

  
  PageProxySetIcon("http://example.org/tests/extensions/cookie/test/image1.png");
}

function obs () {
  this.os = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
  this.os.addObserver(this, "cookie-rejected", false);
}

obs.prototype = {
  observe: function obs_observe (theSubject, theTopic, theData)
  {
    var uri = theSubject.QueryInterface(Components.interfaces.nsIURI);
    var domain = uri.host;

    if (domain == "example.org") {
      ok(true, "foreign favicon cookie was blocked");

      var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefBranch);
      prefs.setIntPref("network.cookie.cookieBehavior", 0);

      this.os.removeObserver(this, "cookie-rejected");
      this.os = null;

      finish();
    }
  }
}










































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  let cp = Cc["@mozilla.org/embedcomp/cookieprompt-service;1"].
           getService(Ci.nsICookiePromptService);

  waitForExplicitFinish();

  function checkRememberOption(expectedDisabled, callback) {
    let observer = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowopened") {
          ww.unregisterNotification(this);
          let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
          win.addEventListener("load", function onLoad(event) {
            win.removeEventListener("load", onLoad, false);

            executeSoon(function() {
              let doc = win.document;
              let remember = doc.getElementById("persistDomainAcceptance");
              ok(remember, "The remember checkbox should exist");

              if (expectedDisabled)
                is(remember.getAttribute("disabled"), "true",
                   "The checkbox should be disabled");
              else
                ok(!remember.hasAttribute("disabled"),
                   "The checkbox should not be disabled");

              win.close();
              callback();
            });
          }, false);
        }
      }
    };
    ww.registerNotification(observer);

    let remember = {};
    const time = (new Date("Jan 1, 2030")).getTime() / 1000;
    let cookie = {
      name: "foo",
      value: "bar",
      isDomain: true,
      host: "mozilla.org",
      path: "/baz",
      isSecure: false,
      expires: time,
      status: 0,
      policy: 0,
      isSession: false,
      expiry: time,
      isHttpOnly: true,
      QueryInterface: function(iid) {
        const validIIDs = [Components.interfaces.nsISupports,
                           Components.interfaces.nsICookie,
                           Components.interfaces.nsICookie2];
        for (var i = 0; i < validIIDs.length; ++i)
          if (iid == validIIDs[i])
            return this;
        throw Components.results.NS_ERROR_NO_INTERFACE;
      }
    };
    cp.cookieDialog(window, cookie, "mozilla.org", 10, false, remember);
  }

  checkRememberOption(false, function() {
    pb.privateBrowsingEnabled = true;
    checkRememberOption(true, function() {
      pb.privateBrowsingEnabled = false;
      checkRememberOption(false, finish);
    });
  });
}









































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  waitForExplicitFinish();

  function openLocation(url, autofilled, callback) {
    let observer = {
      observe: function(aSubject, aTopic, aData) {
        switch (aTopic) {
        case "domwindowopened":
          let dialog = aSubject.QueryInterface(Ci.nsIDOMWindow);
          dialog.addEventListener("load", function () {
            dialog.removeEventListener("load", arguments.callee, false);

            let browser = gBrowser.selectedBrowser;
            browser.addEventListener("load", function() {
              browser.removeEventListener("load", arguments.callee, true);

              is(browser.currentURI.spec, url,
                 "The correct URL should be loaded via the open location dialog");
              executeSoon(callback);
            }, true);

            executeSoon(function() {
              let input = dialog.document.getElementById("dialog.input");
              is(input.value, autofilled, "The input field should be correctly auto-filled");
              input.focus();
              for (let i = 0; i < url.length; ++i)
                EventUtils.synthesizeKey(url[i], {}, dialog);
              EventUtils.synthesizeKey("VK_RETURN", {}, dialog);
            });
          }, false);
          break;

        case "domwindowclosed":
          ww.unregisterNotification(this);
          break;
        }
      }
    };

    ww.registerNotification(observer);
    gPrefService.setIntPref("general.open_location.last_window_choice", 0);
    openDialog("chrome://browser/content/openLocation.xul", "_blank",
               "chrome,titlebar", window);
  }

  gPrefService.clearUserPref("general.open_location.last_url");

  openLocation("http://example.com/", "", function() {
    openLocation("http://example.org/", "http://example.com/", function() {
      
      pb.privateBrowsingEnabled = true;
      openLocation("about:logo", "", function() {
        openLocation("about:buildconfig", "about:logo", function() {
          
          pb.privateBrowsingEnabled = false;
          openLocation("about:blank", "http://example.org/", function() {
            gPrefService.clearUserPref("general.open_location.last_url");
            gPrefService.clearUserPref("general.open_location.last_window_choice");
            finish();
          });
        });
      });
    });
  });
}

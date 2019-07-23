














































let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
         getService(Ci.nsIWindowWatcher);

let windowObserver = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic === "domwindowopened") {
      ww.unregisterNotification(this);
      let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
      win.addEventListener("load", function onLoad(event) {
        win.removeEventListener("load", onLoad, false);
        executeSoon(function () {
          ok(true, "Library has been correctly opened");
          win.close();
          finish();
        });
      }, false);
    }
  }
};

function test() {
  waitForExplicitFinish();
  ww.registerNotification(windowObserver);
  ww.openWindow(null,
                "chrome://browser/content/places/places.xul",
                "",
                "chrome,toolbar=yes,dialog=no,resizable",
                null);
}

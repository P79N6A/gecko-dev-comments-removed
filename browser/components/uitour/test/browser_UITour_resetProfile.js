


"use strict";

let gTestTab;
let gContentAPI;
let gContentWindow;

function test() {
  UITourTest();
}

let tests = [
  
  function test_resetFirefox(done) {
    let winWatcher = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                     getService(Ci.nsIWindowWatcher);
    winWatcher.registerNotification(function onOpen(subj, topic, data) {
      if (topic == "domwindowopened" && subj instanceof Ci.nsIDOMWindow) {
        subj.addEventListener("load", function onLoad() {
          subj.removeEventListener("load", onLoad);
          if (subj.document.documentURI ==
              "chrome://global/content/resetProfile.xul") {
            winWatcher.unregisterNotification(onOpen);
            ok(true, "Observed search manager window open");
            is(subj.opener, window,
               "Reset Firefox event opened a reset profile window.");
            subj.close();
            done();
          }
        });
      }
    });
    gContentAPI.resetFirefox();
  },
];

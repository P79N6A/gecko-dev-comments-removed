



const TEST_PROVIDER_ORIGIN = 'http://example.com';

Cu.import("resource://gre/modules/Services.jsm");

function ensureProvider(workerFunction, cb) {
  let manifest = {
    origin: TEST_PROVIDER_ORIGIN,
    name: "Example Provider",
    workerURL: "data:application/javascript," + encodeURI("let run=" + workerFunction.toSource()) + ";run();"
  };

  ensureSocialEnabled();
  SocialService.addProvider(manifest, function (p) {
    cb(p);
  });
}

function test() {
  waitForExplicitFinish();

  let cbPostTest = function(cb) {
    SocialService.removeProvider(TEST_PROVIDER_ORIGIN, function() {cb()});
  };
  runTests(tests, undefined, cbPostTest);
}

let tests = {
  testNotificationCallback: function(cbnext) {
    let run = function() {
      let testPort, apiPort;
      onconnect = function(e) {
        let port = e.ports[0];
        port.onmessage = function(e) {
          if (e.data.topic == "social.initialize") { 
            apiPort = port;
          } else if (e.data.topic == "test.initialize") { 
            testPort = port;
            apiPort.postMessage({topic: 'social.notification-create',
                                 data: {
                                        id: "the id",
                                        body: 'test notification',
                                        action: 'callback',
                                        actionArgs: { data: "something" }
                                       }
                                });
          } else if (e.data.topic == "social.notification-action") {
            let data = e.data.data;
            let ok = data && data.action == "callback" &&
                     data.actionArgs && e.data.data.actionArgs.data == "something";
            testPort.postMessage({topic: "test.done", data: ok});
          }
        }
      }
    }
    ensureProvider(run, function(provider) {
      if ('@mozilla.org/system-alerts-service;1' in Cc) {
        
        
        
        info("this platform has a system alerts service - test skipped");
        cbnext();
        return;
      }
      if (!("@mozilla.org/alerts-service;1" in Cc)) {
        info("Alerts service does not exist in this application");
        cbnext();
        return;
      }
      var notifier;
      try {
        notifier = Cc["@mozilla.org/alerts-service;1"].
                   getService(Ci.nsIAlertsService);
      } catch (ex) {
        info("Alerts service is not available. (Mac OS X without Growl?)", ex);
        cbnext();
        return;
      }

      provider.port.onmessage = function(e) {
        if (e.data.topic == "test.done") {
          ok(e.data.data, "check the test worked");
          cbnext();
        }
      }
      provider.port.postMessage({topic: "test.initialize"});
      let count = 0;
      
      const ALERT_CHROME_URL = "chrome://global/content/alerts/alert.xul";
      const ALERT_TEXT_LABEL_ID = "alertTextLabel";
      const ALERT_TITLE_LABEL_ID = "alertTitleLabel";
      let findPopup = function() {
        let wenum = Services.ww.getWindowEnumerator();
        while (wenum.hasMoreElements()) {
          let win = wenum.getNext();
          if (win.location.href == ALERT_CHROME_URL) {
            let doc = win.document;
            
            if (doc.getElementById(ALERT_TEXT_LABEL_ID) &&
                doc.getElementById(ALERT_TEXT_LABEL_ID).getAttribute("value")) {
              
              is(doc.getElementById(ALERT_TEXT_LABEL_ID).getAttribute("value"),
                 "test notification",
                 "check the alert label is correct");
              is(doc.getElementById(ALERT_TITLE_LABEL_ID).getAttribute("value"),
                 "Example Provider",
                 "check the alert title is correct");
              
              
              EventUtils.sendMouseEvent({type: "click"}, ALERT_TEXT_LABEL_ID, win);
              return;
            }
          }
        }
        if (count++ > 50) {
          ok(false, "failed to find the notification popup");
          cbnext();
          return;
        }
        executeSoon(findPopup);
      }
      executeSoon(findPopup);
    });
  }
};









































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  let aboutBrowser = gBrowser.selectedBrowser;
  aboutBrowser.addEventListener("load", function () {
    aboutBrowser.removeEventListener("load", arguments.callee, true);

    let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
             getService(Ci.nsIWindowWatcher);
    let observer = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic == "domwindowopened") {
          ww.unregisterNotification(this);

          let win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);
          win.addEventListener("load", function() {
            win.removeEventListener("load", arguments.callee, false);

            let browser = win.gBrowser;
            browser.addEventListener("load", function() {
              browser.removeEventListener("load", arguments.callee, true);
              
              
              step1();
            }, true);
          }, false);
        }
      }
    };
    ww.registerNotification(observer);

    openViewSource();

    function openViewSource() {
      
      document.getElementById("View:PageSource").doCommand();
    }

    function step1() {
      observer = {
        observe: function(aSubject, aTopic, aData) {
          if (aTopic == "domwindowclosed") {
            ok(true, "Entering the private browsing mode should close the view source window");
            ww.unregisterNotification(observer);

            step2();
          }
          else if (aTopic == "domwindowopened")
            ok(false, "Entering the private browsing mode should not open any view source window");
        }
      };
      ww.registerNotification(observer);

      gBrowser.addTabsProgressListener({
        onLocationChange: function() {},
        onProgressChange: function() {},
        onSecurityChange: function() {},
        onStatusChange: function() {},
        onRefreshAttempted: function() {},
        onLinkIconAvailable: function() {},
        onStateChange: function(aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
          if (aStateFlags & (Ci.nsIWebProgressListener.STATE_STOP |
                             Ci.nsIWebProgressListener.STATE_IS_WINDOW)) {
            gBrowser.removeTabsProgressListener(this);

            step3();
          }
        }
      });

      
      pb.privateBrowsingEnabled = true;
    }

    let events = 0, step2, step3;
    step2 = step3 = function() {
      if (++events == 2)
        step4();
    }

    function step4() {
      observer = {
        observe: function(aSubject, aTopic, aData) {
          if (aTopic == "domwindowopened") {
            ww.unregisterNotification(this);

            let win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);
            win.addEventListener("load", function() {
              win.removeEventListener("load", arguments.callee, false);

              let browser = win.getBrowser();
              browser.addEventListener("load", function() {
                browser.removeEventListener("load", arguments.callee, true);
                
                
                step5();
              }, true);
            }, false);
          }
        }
      };
      ww.registerNotification(observer);

      openViewSource();
    }

    function step5() {
      let events = 0;

      observer = {
        observe: function(aSubject, aTopic, aData) {
          if (aTopic == "domwindowclosed") {
            ok(true, "Leaving the private browsing mode should close the existing view source window");
            if (++events == 2)
              ww.unregisterNotification(observer);
          }
          else if (aTopic == "domwindowopened") {
            ok(true, "Leaving the private browsing mode should restore the previous view source window");
            if (++events == 2)
              ww.unregisterNotification(observer);

            let win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);
            win.addEventListener("load", function() {
              win.removeEventListener("load", arguments.callee, false);

              let browser = win.gBrowser;
              browser.addEventListener("load", function() {
                browser.removeEventListener("load", arguments.callee, true);
                
                is(win.content.location.href, "view-source:about:",
                  "The correct view source window should be restored");

                
                win.close();
                gBrowser.removeCurrentTab();
                finish();
              }, true);
            }, false);
          }
        }
      };
      ww.registerNotification(observer);

      
      pb.privateBrowsingEnabled = false;
    }
  }, true);
  aboutBrowser.loadURI("about:");
}

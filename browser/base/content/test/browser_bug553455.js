



const TESTROOT = "http://example.com/browser/toolkit/mozapps/extensions/test/xpinstall/";
const TESTROOT2 = "http://example.org/browser/toolkit/mozapps/extensions/test/xpinstall/";
const SECUREROOT = "https://example.com/browser/toolkit/mozapps/extensions/test/xpinstall/";
const XPINSTALL_URL = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";
const PREF_INSTALL_REQUIREBUILTINCERTS = "extensions.install.requireBuiltInCerts";

var rootDir = getRootDirectory(gTestPath);
var path = rootDir.split('/');
var chromeName = path[0] + '//' + path[2];
var croot = chromeName + "/content/browser/toolkit/mozapps/extensions/test/xpinstall/";
var jar = getJar(croot);
if (jar) {
  var tmpdir = extractJarToTmp(jar);
  croot = 'file://' + tmpdir.path + '/';
}
const CHROMEROOT = croot;

var gApp = document.getElementById("bundle_brand").getString("brandShortName");
var gVersion = Services.appinfo.version;
var check_notification;

function wait_for_notification(aCallback) {
  info("Waiting for notification");
  check_notification = function() {
    PopupNotifications.panel.removeEventListener("popupshown", check_notification, false);
    info("Saw notification");
    is(PopupNotifications.panel.childNodes.length, 1, "Should be only one notification");
    aCallback(PopupNotifications.panel);
  };
  PopupNotifications.panel.addEventListener("popupshown", check_notification, false);
}

function wait_for_notification_close(aCallback) {
  info("Waiting for notification to close");
  PopupNotifications.panel.addEventListener("popuphidden", function() {
    PopupNotifications.panel.removeEventListener("popuphidden", arguments.callee, false);
    aCallback();
  }, false);
}

function wait_for_install_dialog(aCallback) {
  info("Waiting for install dialog");
  Services.wm.addListener({
    onOpenWindow: function(aXULWindow) {
      info("Install dialog opened, waiting for focus");
      Services.wm.removeListener(this);

      var domwindow = aXULWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIDOMWindowInternal);
      waitForFocus(function() {
        info("Saw install dialog");
        is(domwindow.document.location.href, XPINSTALL_URL, "Should have seen the right window open");

        
        var button = domwindow.document.documentElement.getButton("accept");
        button.disabled = false;

        aCallback(domwindow);
      }, domwindow);
    },

    onCloseWindow: function(aXULWindow) {
    },

    onWindowTitleChange: function(aXULWindow, aNewTitle) {
    }
  });
}

function setup_redirect(aSettings) {
  var url = "https://example.com/browser/toolkit/mozapps/extensions/test/xpinstall/redirect.sjs?mode=setup";
  for (var name in aSettings) {
    url += "&" + name + "=" + aSettings[name];
  }

  var req = new XMLHttpRequest();
  req.open("GET", url, false);
  req.send(null);
}

var TESTS = [
function test_disabled_install() {
  Services.prefs.setBoolPref("xpinstall.enabled", false);

  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "xpinstall-disabled-notification", "Should have seen installs disabled");
    is(notification.button.label, "Enable", "Should have seen the right button");
    is(notification.getAttribute("label"),
       "Software installation is currently disabled. Click Enable and try again.");

    wait_for_notification_close(function() {
      try {
        Services.prefs.getBoolPref("xpinstall.disabled");
        ok(false, "xpinstall.disabled should not be set");
      }
      catch (e) {
        ok(true, "xpinstall.disabled should not be set");
      }

      gBrowser.removeTab(gBrowser.selectedTab);

      AddonManager.getAllInstalls(function(aInstalls) {
        is(aInstalls.length, 1, "Should have been one install created");
        aInstalls[0].cancel();

        runNextTest();
      });
    });

    
    EventUtils.synthesizeMouseAtCenter(notification.button, {});
  });

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "unsigned.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_blocked_install() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-install-blocked-notification", "Should have seen the install blocked");
    is(notification.button.label, "Allow", "Should have seen the right button");
    is(notification.getAttribute("label"),
       gApp + " prevented this site (example.com) from asking you to install " +
       "software on your computer.",
       "Should have seen the right message");

    
    wait_for_install_dialog(function(aWindow) {
      
      wait_for_notification(function(aPanel) {
        let notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-complete-notification", "Should have seen the install complete");
        is(notification.button.label, "Restart Now", "Should have seen the right button");
        is(notification.getAttribute("label"),
           "XPI Test will be installed after you restart " + gApp + ".",
           "Should have seen the right message");

        AddonManager.getAllInstalls(function(aInstalls) {
        is(aInstalls.length, 1, "Should be one pending install");
          aInstalls[0].cancel();

          wait_for_notification_close(runNextTest);
          gBrowser.removeTab(gBrowser.selectedTab);
        });
      });

      aWindow.document.documentElement.acceptDialog();
    });

    
    EventUtils.synthesizeMouse(notification.button, 20, 10, {});

    
    ok(PopupNotifications.isPanelOpen, "Notification should still be open");
    notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

  });

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "unsigned.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_whitelisted_install() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    wait_for_install_dialog(function(aWindow) {
      
      wait_for_notification(function(aPanel) {
        let notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-complete-notification", "Should have seen the install complete");
        is(notification.button.label, "Restart Now", "Should have seen the right button");
        is(notification.getAttribute("label"),
           "XPI Test will be installed after you restart " + gApp + ".",
           "Should have seen the right message");

        AddonManager.getAllInstalls(function(aInstalls) {
          is(aInstalls.length, 1, "Should be one pending install");
          aInstalls[0].cancel();

          Services.perms.remove("example.com", "install");
          wait_for_notification_close(runNextTest);
          gBrowser.removeTab(gBrowser.selectedTab);
        });
      });

      aWindow.document.documentElement.acceptDialog();
    });
  });

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "unsigned.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_failed_download() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    wait_for_notification(function(aPanel) {
      let notification = aPanel.childNodes[0];
      is(notification.id, "addon-install-failed-notification", "Should have seen the install fail");
      is(notification.getAttribute("label"),
         "The add-on could not be downloaded because of a connection failure " +
         "on example.com.",
         "Should have seen the right message");

      Services.perms.remove("example.com", "install");
      wait_for_notification_close(runNextTest);
      gBrowser.removeTab(gBrowser.selectedTab);
    });
  });

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "missing.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_corrupt_file() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    wait_for_notification(function(aPanel) {
      let notification = aPanel.childNodes[0];
      is(notification.id, "addon-install-failed-notification", "Should have seen the install fail");
      is(notification.getAttribute("label"),
         "The add-on downloaded from example.com could not be installed " +
         "because it appears to be corrupt.",
         "Should have seen the right message");

      Services.perms.remove("example.com", "install");
      wait_for_notification_close(runNextTest);
      gBrowser.removeTab(gBrowser.selectedTab);
    });
  });

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "corrupt.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_incompatible() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    wait_for_notification(function(aPanel) {
      let notification = aPanel.childNodes[0];
      is(notification.id, "addon-install-failed-notification", "Should have seen the install fail");
      is(notification.getAttribute("label"),
         "XPI Test could not be installed because it is not compatible with " +
         gApp + " " + gVersion + ".",
         "Should have seen the right message");

      Services.perms.remove("example.com", "install");
      wait_for_notification_close(runNextTest);
      gBrowser.removeTab(gBrowser.selectedTab);
    });
  });

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "incompatible.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_restartless() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    wait_for_install_dialog(function(aWindow) {
      
      wait_for_notification(function(aPanel) {
        let notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-complete-notification", "Should have seen the install complete");
        is(notification.button.label, "Open Add-ons Manager", "Should have seen the right button");
        is(notification.getAttribute("label"),
           "XPI Test has been installed successfully.",
           "Should have seen the right message");

        AddonManager.getAllInstalls(function(aInstalls) {
          is(aInstalls.length, 0, "Should be no pending installs");

          AddonManager.getAddonByID("restartless-xpi@tests.mozilla.org", function(aAddon) {
            aAddon.uninstall();

            Services.perms.remove("example.com", "install");
            wait_for_notification_close(runNextTest);
            gBrowser.removeTab(gBrowser.selectedTab);
          });
        });
      });

      aWindow.document.documentElement.acceptDialog();
    });
  });

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "restartless.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_multiple() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    wait_for_install_dialog(function(aWindow) {
      
      wait_for_notification(function(aPanel) {
        let notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-complete-notification", "Should have seen the install complete");
        is(notification.button.label, "Restart Now", "Should have seen the right button");
        is(notification.getAttribute("label"),
           "2 add-ons will be installed after you restart " + gApp + ".",
           "Should have seen the right message");

        AddonManager.getAllInstalls(function(aInstalls) {
          is(aInstalls.length, 1, "Should be one pending install");
          aInstalls[0].cancel();

          AddonManager.getAddonByID("restartless-xpi@tests.mozilla.org", function(aAddon) {
            aAddon.uninstall();

            Services.perms.remove("example.com", "install");
            wait_for_notification_close(runNextTest);
            gBrowser.removeTab(gBrowser.selectedTab);
          });
        });
      });

      aWindow.document.documentElement.acceptDialog();
    });
  });

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "Unsigned XPI": "unsigned.xpi",
    "Restartless XPI": "restartless.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_url() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    wait_for_install_dialog(function(aWindow) {
      
      wait_for_notification(function(aPanel) {
        let notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-complete-notification", "Should have seen the install complete");
        is(notification.button.label, "Restart Now", "Should have seen the right button");
        is(notification.getAttribute("label"),
           "XPI Test will be installed after you restart " + gApp + ".",
           "Should have seen the right message");

        AddonManager.getAllInstalls(function(aInstalls) {
          is(aInstalls.length, 1, "Should be one pending install");
          aInstalls[0].cancel();

          wait_for_notification_close(runNextTest);
          gBrowser.removeTab(gBrowser.selectedTab);
        });
      });

      aWindow.document.documentElement.acceptDialog();
    });
  });

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "unsigned.xpi");
},

function test_localfile() {
  
  Services.obs.addObserver(function() {
    Services.obs.removeObserver(arguments.callee, "addon-install-failed");

    
    executeSoon(function() {
      let notification = PopupNotifications.panel.childNodes[0];
      is(notification.id, "addon-install-failed-notification", "Should have seen the install fail");
      is(notification.getAttribute("label"),
         "This add-on could not be installed because it appears to be corrupt.",
         "Should have seen the right message");

      wait_for_notification_close(runNextTest);
      gBrowser.removeTab(gBrowser.selectedTab);
    });
  }, "addon-install-failed", false);

  var cr = Components.classes["@mozilla.org/chrome/chrome-registry;1"]
                     .getService(Components.interfaces.nsIChromeRegistry);
  try {
    var path = cr.convertChromeURL(makeURI(CHROMEROOT + "corrupt.xpi")).spec;
  } catch (ex) {
    var path = CHROMEROOT + "corrupt.xpi";
  }
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(path);
},

function test_wronghost() {
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.addEventListener("load", function() {
    if (gBrowser.currentURI.spec != TESTROOT2 + "enabled.html")
      return;

    gBrowser.removeEventListener("load", arguments.callee, true);

    
    wait_for_notification(function(aPanel) {
      let notification = aPanel.childNodes[0];
      is(notification.id, "addon-progress-notification", "Should have seen the progress notification");
      
      wait_for_notification(function(aPanel) {
        let notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-failed-notification", "Should have seen the install fail");
        is(notification.getAttribute("label"),
           "The add-on downloaded from example.com could not be installed " +
           "because it appears to be corrupt.",
           "Should have seen the right message");

        wait_for_notification_close(runNextTest);
        gBrowser.removeTab(gBrowser.selectedTab);
      });
    });

    gBrowser.loadURI(TESTROOT + "corrupt.xpi");
  }, true);
  gBrowser.loadURI(TESTROOT2 + "enabled.html");
},

function test_reload() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    wait_for_install_dialog(function(aWindow) {
      
      wait_for_notification(function(aPanel) {
        let notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-complete-notification", "Should have seen the install complete");
        is(notification.button.label, "Restart Now", "Should have seen the right button");
        is(notification.getAttribute("label"),
           "XPI Test will be installed after you restart " + gApp + ".",
           "Should have seen the right message");

        function test_fail() {
          ok(false, "Reloading should not have hidden the notification");
        }

        PopupNotifications.panel.addEventListener("popuphiding", test_fail, false);

        gBrowser.addEventListener("load", function() {
          if (gBrowser.currentURI.spec != TESTROOT2 + "enabled.html")
            return;

          gBrowser.removeEventListener("load", arguments.callee, true);

          PopupNotifications.panel.removeEventListener("popuphiding", test_fail, false);

          AddonManager.getAllInstalls(function(aInstalls) {
            is(aInstalls.length, 1, "Should be one pending install");
            aInstalls[0].cancel();

            Services.perms.remove("example.com", "install");
            wait_for_notification_close(runNextTest);
            gBrowser.removeTab(gBrowser.selectedTab);
          });
        }, true);
        gBrowser.loadURI(TESTROOT2 + "enabled.html");
      });

      aWindow.document.documentElement.acceptDialog();
    });
  });

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "Unsigned XPI": "unsigned.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_theme() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    wait_for_install_dialog(function(aWindow) {
      
      wait_for_notification(function(aPanel) {
        let notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-complete-notification", "Should have seen the install complete");
        is(notification.button.label, "Restart Now", "Should have seen the right button");
        is(notification.getAttribute("label"),
           "Theme Test will be installed after you restart " + gApp + ".",
           "Should have seen the right message");

        AddonManager.getAddonByID("{972ce4c6-7e08-4474-a285-3208198ce6fd}", function(aAddon) {
          ok(aAddon.userDisabled, "Should be switching away from the default theme.");
          
          aAddon.userDisabled = false;

          AddonManager.getAddonByID("theme-xpi@tests.mozilla.org", function(aAddon) {
            isnot(aAddon, null, "Test theme will have been installed");
            aAddon.uninstall();

            Services.perms.remove("example.com", "install");
            wait_for_notification_close(runNextTest);
            gBrowser.removeTab(gBrowser.selectedTab);
          });
        });
      });

      aWindow.document.documentElement.acceptDialog();
    });
  });

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "Theme XPI": "theme.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_renotify_blocked() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-install-blocked-notification", "Should have seen the install blocked");

    wait_for_notification_close(function () {
      info("Timeouts after this probably mean bug 589954 regressed");
      executeSoon(function () {
        wait_for_notification(function(aPanel) {
          let notification = aPanel.childNodes[0];
          is(notification.id, "addon-install-blocked-notification",
             "Should have seen the install blocked - 2nd time");

          AddonManager.getAllInstalls(function(aInstalls) {
          is(aInstalls.length, 2, "Should be two pending installs");
            aInstalls[0].cancel();
            aInstalls[1].cancel();

            info("Closing browser tab");
            wait_for_notification_close(runNextTest);
            gBrowser.removeTab(gBrowser.selectedTab);
          });
        });

        gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
      });
    });

    
    aPanel.hidePopup();
  });

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "unsigned.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_renotify_installed() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    wait_for_install_dialog(function(aWindow) {
      
      wait_for_notification(function(aPanel) {
        let notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-complete-notification", "Should have seen the install complete");

        
        wait_for_notification_close(function () {
          
          executeSoon(function () {
            
            wait_for_notification(function(aPanel) {
              let notification = aPanel.childNodes[0];
              is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

              
              wait_for_install_dialog(function(aWindow) {
                info("Timeouts after this probably mean bug 589954 regressed");

                
                wait_for_notification(function(aPanel) {
                  let notification = aPanel.childNodes[0];
                  is(notification.id, "addon-install-complete-notification", "Should have seen the second install complete");

                  AddonManager.getAllInstalls(function(aInstalls) {
                  is(aInstalls.length, 1, "Should be one pending installs");
                    aInstalls[0].cancel();

                    Services.perms.remove("example.com", "install");
                    wait_for_notification_close(runNextTest);
                    gBrowser.removeTab(gBrowser.selectedTab);
                  });
                });

                aWindow.document.documentElement.acceptDialog();
              });
            });

            gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
          });
        });

        
        aPanel.hidePopup();
      });

      aWindow.document.documentElement.acceptDialog();
    });
  });

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "unsigned.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_cancel_restart() {
  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    let anchor = document.getElementById("addons-notification-icon");
    EventUtils.synthesizeMouseAtCenter(anchor, {});
    
    EventUtils.synthesizeMouseAtCenter(anchor, {});

    ok(PopupNotifications.isPanelOpen, "Notification should still be open");
    is(PopupNotifications.panel.childNodes.length, 1, "Should be only one notification");
    isnot(notification, aPanel.childNodes[0], "Should have reconstructed the notification UI");
    notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");
    let button = document.getAnonymousElementByAttribute(notification, "anonid", "cancel");

    
    EventUtils.synthesizeMouse(button, 2, 2, {});

    
    notification = aPanel.childNodes[0];
    is(notification.id, "addon-install-cancelled-notification", "Should have seen the cancelled notification");

    
    wait_for_install_dialog(function(aWindow) {
      
      wait_for_notification(function(aPanel) {
        let notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-complete-notification", "Should have seen the install complete");
        is(notification.button.label, "Restart Now", "Should have seen the right button");
        is(notification.getAttribute("label"),
           "XPI Test will be installed after you restart " + gApp + ".",
           "Should have seen the right message");

        AddonManager.getAllInstalls(function(aInstalls) {
          is(aInstalls.length, 1, "Should be one pending install");
          aInstalls[0].cancel();

          Services.perms.remove("example.com", "install");
          wait_for_notification_close(runNextTest);
          gBrowser.removeTab(gBrowser.selectedTab);
        });
      });

      aWindow.document.documentElement.acceptDialog();
    });

    
    EventUtils.synthesizeMouse(notification.button, 20, 10, {});

    
    ok(PopupNotifications.isPanelOpen, "Notification should still be open");
    is(PopupNotifications.panel.childNodes.length, 1, "Should be only one notification");
    notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");
  });

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "unsigned.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
},

function test_failed_security() {
  Services.prefs.setBoolPref(PREF_INSTALL_REQUIREBUILTINCERTS, false);

  setup_redirect({
    "Location": TESTROOT + "unsigned.xpi"
  });

  
  wait_for_notification(function(aPanel) {
    let notification = aPanel.childNodes[0];
    is(notification.id, "addon-install-blocked-notification", "Should have seen the install blocked");

    
    EventUtils.synthesizeMouse(notification.button, 20, 10, {});

    
    ok(PopupNotifications.isPanelOpen, "Notification should still be open");
    is(PopupNotifications.panel.childNodes.length, 1, "Should be only one notification");
    notification = aPanel.childNodes[0];
    is(notification.id, "addon-progress-notification", "Should have seen the progress notification");

    
    Services.obs.addObserver(function() {
      Services.obs.removeObserver(arguments.callee, "addon-install-failed");

      function waitForSingleNotification() {
        
        ok(PopupNotifications.isPanelOpen, "Notification should still be open");
        if (PopupNotifications.panel.childNodes.length == 2) {
          executeSoon(waitForSingleNotification);
          return;
        }

        is(PopupNotifications.panel.childNodes.length, 1, "Should be only one notification");
        notification = aPanel.childNodes[0];
        is(notification.id, "addon-install-failed-notification", "Should have seen the install fail");

        Services.prefs.setBoolPref(PREF_INSTALL_REQUIREBUILTINCERTS, true);
        wait_for_notification_close(runNextTest);
        gBrowser.removeTab(gBrowser.selectedTab);
      }

      
      
      executeSoon(waitForSingleNotification);
    }, "addon-install-failed", false);
  });

  var triggers = encodeURIComponent(JSON.stringify({
    "XPI": "redirect.sjs?mode=redirect"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(SECUREROOT + "installtrigger.html?" + triggers);
}
];

var gTestStart = null;

function runNextTest() {
  if (gTestStart)
    info("Test part took " + (Date.now() - gTestStart) + "ms");

  ok(!PopupNotifications.isPanelOpen, "Notification should be closed");

  AddonManager.getAllInstalls(function(aInstalls) {
    is(aInstalls.length, 0, "Should be no active installs");

    if (TESTS.length == 0) {
      finish();
      return;
    }

    info("Running " + TESTS[0].name);
    gTestStart = Date.now();
    TESTS.shift()();
  });
};

var XPInstallObserver = {
  observe: function (aSubject, aTopic, aData) {
    var installInfo = aSubject.QueryInterface(Components.interfaces.amIWebInstallInfo);
    info("Observed " + aTopic + " for " + installInfo.installs.length + " installs");
    installInfo.installs.forEach(function(aInstall) {
      info("Install of " + aInstall.sourceURI.spec + " was in state " + aInstall.state);
    });
  }
};

function test() {
  requestLongerTimeout(4);
  waitForExplicitFinish();

  Services.prefs.setBoolPref("extensions.logging.enabled", true);

  Services.obs.addObserver(XPInstallObserver, "addon-install-started", false);
  Services.obs.addObserver(XPInstallObserver, "addon-install-blocked", false);
  Services.obs.addObserver(XPInstallObserver, "addon-install-failed", false);
  Services.obs.addObserver(XPInstallObserver, "addon-install-complete", false);

  registerCleanupFunction(function() {
    
    TESTS = [];
    PopupNotifications.panel.removeEventListener("popupshown", check_notification, false);

    AddonManager.getAllInstalls(function(aInstalls) {
      aInstalls.forEach(function(aInstall) {
        aInstall.cancel();
      });
    });

    Services.prefs.clearUserPref("extensions.logging.enabled");

    Services.obs.removeObserver(XPInstallObserver, "addon-install-started");
    Services.obs.removeObserver(XPInstallObserver, "addon-install-blocked");
    Services.obs.removeObserver(XPInstallObserver, "addon-install-failed");
    Services.obs.removeObserver(XPInstallObserver, "addon-install-complete");
  });

  runNextTest();
}

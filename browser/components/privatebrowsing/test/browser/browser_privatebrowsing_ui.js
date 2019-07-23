







































function test() {
  
  let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let observer = {
    observe: function (aSubject, aTopic, aData) {
      if (aTopic == "private-browsing")
        this.data = aData;
    },
    data: null
  };
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.addObserver(observer, "private-browsing", false);
  let pbMenuItem = document.getElementById("privateBrowsingItem");
  
  let blankTab = gBrowser.addTab();
  gBrowser.selectedTab = blankTab;
  let originalTitle = document.title;
  let privateBrowsingTitle = document.documentElement.getAttribute("titlemodifier_privatebrowsing");
  waitForExplicitFinish();

  
  ok(gPrivateBrowsingUI, "The gPrivateBrowsingUI object exists");
  ok(pbMenuItem, "The Private Browsing menu item exists");
  ok(!pbMenuItem.hasAttribute("checked"), "The Private Browsing menu item is not checked initially");
  gPrivateBrowsingUI.toggleMode();
  
  is(observer.data, "enter", "Private Browsing mode was activated using the gPrivateBrowsingUI object");
  ok(pbMenuItem.hasAttribute("checked"), "The Private Browsing menu item was correctly checked");
  gPrivateBrowsingUI.toggleMode()
  
  is(observer.data, "exit", "Private Browsing mode was deactivated using the gPrivateBrowsingUI object");
  ok(!pbMenuItem.hasAttribute("checked"), "The Private Browsing menu item was correctly unchecked");

  
  window.focus();
  let timer = Cc["@mozilla.org/timer;1"].
              createInstance(Ci.nsITimer);
  timer.initWithCallback({
    notify: function(timer) {
      
      let toolsKey = document.getElementById("tools-menu").getAttribute("accesskey");
      let pbKey = pbMenuItem.getAttribute("accesskey");

      
      function accessKeyModifier () {
        switch (gPrefService.getIntPref("ui.key.generalAccessKey")) {
        case -1:
          let chromeAccessKey = gPrefService.getIntPref("ui.key.chromeAccess");
          if (chromeAccessKey == 0)
            ok(false, "ui.key.chromeAccess was set to 0, so access keys for chrome are disabled");
          else
            return {
              shiftKey: (chromeAccessKey & 1) != 0,
              accelKey: (chromeAccessKey & 2) != 0,
              altKey: (chromeAccessKey & 4) != 0,
              metaKey: (chromeAccessKey & 8) != 0
            };
          break;

        case Ci.nsIDOMKeyEvent.DOM_VK_SHIFT:
          return { shiftKey: true };
        case Ci.nsIDOMKeyEvent.DOM_VK_CONTROL:
          return { accelKey: true };
        case Ci.nsIDOMKeyEvent.DOM_VK_ALT:
          return { altKey: true };
        case Ci.nsIDOMKeyEvent.DOM_VK_META:
          return { metaKey: true };
        default:
          ok(false, "Invalid value for the ui.key.generalAccessKey pref: " +
            gPrefService.getIntPref("ui.key.generalAccessKey"));
        }
      }

      let popup = document.getElementById("menu_ToolsPopup");
      
      popup.addEventListener("popupshown", function () {
        this.removeEventListener("popupshown", arguments.callee, false);

        this.addEventListener("popuphidden", function () {
          this.removeEventListener("popuphidden", arguments.callee, false);

          
          is(observer.data, "enter", "Private Browsing mode was activated using the menu");
          
          is(document.title, privateBrowsingTitle, "Private browsing mode has correctly changed the title");

          
          this.addEventListener("popupshown", function () {
            this.removeEventListener("popupshown", arguments.callee, false);

            this.addEventListener("popuphidden", function () {
              this.removeEventListener("popuphidden", arguments.callee, false);

              
              is(observer.data, "exit", "Private Browsing mode was deactivated using the menu");
              
              is(document.title, originalTitle, "Private browsing mode has correctly restored the title");

              
              let cmd = document.getElementById("Tools:PrivateBrowsing");
              isnot(cmd, null, "XUL command object for the private browsing service exists");
              var func = new Function("", cmd.getAttribute("oncommand"));
              func.call(cmd);
              
              is(observer.data, "enter", "Private Browsing mode was activated using the command object");
              
              is(document.title, privateBrowsingTitle, "Private browsing mode has correctly changed the title");
              func.call(cmd);
              
              is(observer.data, "exit", "Private Browsing mode was deactivated using the command object");
              
              is(document.title, originalTitle, "Private browsing mode has correctly restored the title");

              
              gBrowser.removeTab(blankTab);
              os.removeObserver(observer, "private-browsing");
              prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
              finish();
            }, false);
            EventUtils.synthesizeKey(pbKey, accessKeyModifier());
          }, false);
          EventUtils.synthesizeKey(toolsKey, accessKeyModifier());
        }, false);
        EventUtils.synthesizeKey(pbKey, accessKeyModifier());
      }, false);
      EventUtils.synthesizeKey(toolsKey, accessKeyModifier());
    }
  }, 2000, Ci.nsITimer.TYPE_ONE_SHOT);
}

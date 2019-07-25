





































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-console.html";

let pb = Cc["@mozilla.org/privatebrowsing;1"].
         getService(Ci.nsIPrivateBrowsingService);

registerCleanupFunction(function() {
  Services.prefs.clearUserPref("devtools.gcli.enable");
});

function test() {
  Services.prefs.setBoolPref("devtools.gcli.enable", false);
  addTab("data:text/html,Web Console test for bug 618311 (private browsing)");

  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    registerCleanupFunction(function() {
      pb.privateBrowsingEnabled = false;
      pb = null;
    });

    ok(!pb.privateBrowsingEnabled, "private browsing is not enabled");

    togglePBAndThen(function() {
      ok(pb.privateBrowsingEnabled, "private browsing is enabled");

      HUDService.activateHUDForContext(gBrowser.selectedTab);
      content.location = TEST_URI;
      gBrowser.selectedBrowser.addEventListener("load", tabLoaded, true);
    });
  }, true);
}

function tabLoaded() {
  gBrowser.selectedBrowser.removeEventListener("load", tabLoaded, true);

  let hudId = HUDService.getHudIdByWindow(content);
  let HUD = HUDService.hudReferences[hudId];

  HUD.jsterm.execute("document");

  let networkMessage = HUD.outputNode.querySelector(".webconsole-msg-network");
  ok(networkMessage, "found network message");

  let networkLink = networkMessage.querySelector(".webconsole-msg-link");
  ok(networkLink, "found network message link");

  let jstermMessage = HUD.outputNode.querySelector(".webconsole-msg-output");
  ok(jstermMessage, "found output message");

  let popupset = document.getElementById("mainPopupSet");
  ok(popupset, "found #mainPopupSet");

  let popupsShown = 0;
  let hiddenPopups = 0;

  let onpopupshown = function() {
    popupsShown++;
    if (popupsShown == 2) {
      document.removeEventListener("popupshown", onpopupshown, false);

      executeSoon(function() {
        
        let popups = popupset.querySelectorAll("panel[hudId=" + hudId + "]");
        is(popups.length, 2, "found two popups");

        document.addEventListener("popuphidden", onpopuphidden, false);

        registerCleanupFunction(function() {
          is(hiddenPopups, 2, "correct number of popups hidden");
          if (hiddenPopups != 2) {
            document.removeEventListener("popuphidden", onpopuphidden, false);
          }
        });

        
        pb.privateBrowsingEnabled = !pb.privateBrowsingEnabled;
      });
    }
  };

  let onpopuphidden = function(aEvent) {
    
    if (!aEvent.target.hasAttribute("hudId")) {
      return;
    }

    hiddenPopups++;
    if (hiddenPopups == 2) {
      document.removeEventListener("popuphidden", onpopuphidden, false);

      executeSoon(function() {
        let popups = popupset.querySelectorAll("panel[hudId=" + hudId + "]");
        is(popups.length, 0, "no popups found");

        ok(!pb.privateBrowsingEnabled, "private browsing is not enabled");

        executeSoon(finishTest);
      });
    }
  };

  document.addEventListener("popupshown", onpopupshown, false);

  registerCleanupFunction(function() {
    is(popupsShown, 2, "correct number of popups shown");
    if (popupsShown != 2) {
      document.removeEventListener("popupshown", onpopupshown, false);
    }
  });

  
  EventUtils.synthesizeMouse(networkLink, 2, 2, {});
  EventUtils.synthesizeMouse(jstermMessage, 2, 2, {});
}

function togglePBAndThen(callback) {
  function pbObserver(aSubject, aTopic, aData) {
    if (aTopic != "private-browsing-transition-complete") {
      return;
    }

    Services.obs.removeObserver(pbObserver, "private-browsing-transition-complete");
    afterAllTabsLoaded(callback);
  }

  Services.obs.addObserver(pbObserver, "private-browsing-transition-complete", false);
  pb.privateBrowsingEnabled = !pb.privateBrowsingEnabled;
}

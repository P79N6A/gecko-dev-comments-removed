







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const TEST_URI = "http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/ctxmenu.html";

  waitForExplicitFinish();

  function checkBlockImageMenuItem(expectedHidden, callback) {
    let tab = gBrowser.addTab();
    gBrowser.selectedTab = tab;
    let browser = gBrowser.getBrowserForTab(tab);
    browser.addEventListener("load", function() {
      browser.removeEventListener("load", arguments.callee, true);

      executeSoon(function() {
        let contextMenu = document.getElementById("contentAreaContextMenu");
        let blockImage = document.getElementById("context-blockimage");
        let image = browser.contentDocument.getElementsByTagName("img")[0];
        ok(image, "The content image should be accessible");

        contextMenu.addEventListener("popupshown", function() {
          contextMenu.removeEventListener("popupshown", arguments.callee, false);

          is(blockImage.hidden, expectedHidden,
             "The Block Image menu item should " + (expectedHidden ? "" : "not ") + "be hidden");
          contextMenu.hidePopup();
          gBrowser.removeTab(tab);
          callback();
        }, false);

        document.popupNode = image;
        EventUtils.synthesizeMouse(image, 2, 2,
                                   {type: "contextmenu", button: 2},
                                   browser.contentWindow);
      });
    }, true);
    browser.loadURI(TEST_URI);
  }

  checkBlockImageMenuItem(false, function() {
    pb.privateBrowsingEnabled = true;
    checkBlockImageMenuItem(true, function() {
      pb.privateBrowsingEnabled = false;
      checkBlockImageMenuItem(false, finish);
    });
  });
}

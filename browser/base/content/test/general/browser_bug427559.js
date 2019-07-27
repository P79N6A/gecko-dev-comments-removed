










let testPage = 'data:text/html,<body><button onblur="this.parentNode.removeChild(this);"><script>document.body.firstChild.focus();</script></body>';

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  var browser = gBrowser.selectedBrowser;

  browser.addEventListener("load", function () {
    browser.removeEventListener("load", arguments.callee, true);
    executeSoon(function () {
      var testPageWin = content;

      is(browser.contentDocumentAsCPOW.activeElement.localName, "button", "button is focused");

      addEventListener("focus", function focusedWindow(event) {
        if (!String(event.target.location).startsWith("data:"))
          return;

        removeEventListener("focus", focusedWindow, true);

        
        is(browser.contentDocumentAsCPOW.activeElement.localName, "body", "body is focused");

        gBrowser.removeCurrentTab();
        finish();
      }, true);

      
      
      
      gBrowser.selectedTab = gBrowser.addTab();
      gBrowser.removeCurrentTab();
    });
  }, true);

  content.location = testPage;
}












































let testPage = 'data:text/html,<body><button onblur="this.parentNode.removeChild(this);"><script>document.body.firstChild.focus();</script></body>';

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();

  gBrowser.selectedBrowser.addEventListener("load", function () {
    setTimeout(function () {
      var testPageWin = content;

      
      
      
      gBrowser.selectedTab = gBrowser.addTab();
      gBrowser.removeCurrentTab();

      
      is(document.commandDispatcher.focusedWindow, testPageWin,
         "content window is focused");

      gBrowser.removeCurrentTab();
      finish();
    }, 0);
  }, true);

  content.location = testPage;
}

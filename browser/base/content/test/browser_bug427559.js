










































let testPage = 'data:text/html,<body><button onblur="this.parentNode.removeChild(this);"><script>document.body.firstChild.focus();</script></body>';

function test() {
  waitForExplicitFinish();

  
  let testTab = gBrowser.addTab();
  gBrowser.selectedTab = testTab;
  let testBrowser = gBrowser.getBrowserForTab(testTab);

  
  testBrowser.addEventListener("load", function() setTimeout(function() {
    
    
    
    let emptyTab = gBrowser.addTab();
    gBrowser.selectedTab = emptyTab;
    gBrowser.removeCurrentTab();
    gBrowser.selectedTab = testTab;

    
    is(document.commandDispatcher.focusedWindow, window.content,
       "content window is focused");
    gBrowser.removeCurrentTab();
    finish();
  }, 0), true);

  
  testBrowser.contentWindow.location = testPage;
}

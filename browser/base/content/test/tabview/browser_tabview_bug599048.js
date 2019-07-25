


let cw;

function test() {
  waitForExplicitFinish();

  showTabView(function() {
    cw = TabView.getContentWindow();

    whenSearchIsEnabled(function() {
      ok(cw.Search.isEnabled(), "The search is disabled");

      
      newWindowWithTabView(function(win) {
        registerCleanupFunction(function() {
          win.close();
          hideTabView();
        });
        testClickOnSearchShade(win);
      });
    });

    EventUtils.synthesizeKey("VK_SLASH", {}, cw);
  });
}

function testClickOnSearchShade(win) {
  
  let searchshade = cw.document.getElementById("searchshade");
  EventUtils.sendMouseEvent({ type: "click" }, searchshade, cw);

  waitForFocus(function() {
    ok(cw.Search.isEnabled(), "The search is still enabled after the search shade is clicked");
    testFocusInactiveWindow(win, cw);
  });
}

function testFocusInactiveWindow(win) {
  win.focus();
  
  window.focus();

  
  executeSoon(function() {
    ok(cw.Search.isEnabled(), "The search is still enabled when inactive window has focus");

    whenSearchIsDisabled(function() {
      hideTabView(finish);
    });

    let searchshade = cw.document.getElementById("searchshade");
    EventUtils.synthesizeMouseAtCenter(searchshade, {}, cw);
});
}


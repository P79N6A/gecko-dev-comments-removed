



function test() {
  runTests();
}

gTests.push({
  desc: "Onscreen keyboard tests",
  run: function() {
    
    
    
    
    
    let originalUtils = window.MetroUtils;
    window.MetroUtils = {
      keyboardHeight: 0,
      keyboardVisible: false
    };
    registerCleanupFunction(function() {
      window.MetroUtils = originalUtils;
    });

    let tab = yield addTab(chromeRoot + "browser_onscreen_keyboard.html");
    
    
    ContextUI.dismiss();

    let doc = tab.browser.contentDocument;
    let text = doc.getElementById("text")
    let rect0 = text.getBoundingClientRect();
    let rect0browserY = Math.floor(tab.browser.ptClientToBrowser(rect0.left, rect0.top).y);

    
    SelectionHelperUI.attachToCaret(tab.browser, rect0.left + 5, rect0.top + 5);

    
    MetroUtils.keyboardHeight = 100;
    MetroUtils.keyboardVisible = true;
    Services.obs.notifyObservers(null, "metro_softkeyboard_shown", null);

    let event = yield waitForEvent(window, "MozDeckOffsetChanged");
    is(event.detail, 100, "deck offset by keyboard height");

    let rect1 = text.getBoundingClientRect();
    let rect1browserY = Math.floor(tab.browser.ptClientToBrowser(rect1.left, rect1.top).y);
    is(rect1browserY, rect0browserY + 100, "text field moves up by 100px");

    
    MetroUtils.keyboardHeight = 0;
    MetroUtils.keyboardVisible = false;
    Services.obs.notifyObservers(null, "metro_softkeyboard_hidden", null);

    yield waitForEvent(window, "MozDeckOffsetChanged");

    finish();
  }
});

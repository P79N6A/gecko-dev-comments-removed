



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
    text.focus();

    
    MetroUtils.keyboardHeight = 100;
    MetroUtils.keyboardVisible = true;
    Services.obs.notifyObservers(null, "metro_softkeyboard_shown", null);

    let rect1 = text.getBoundingClientRect();
    is(rect1.top, rect0.top - 100, "text field moves up by 100px");

    
    MetroUtils.keyboardHeight = 0;
    MetroUtils.keyboardVisible = false;
    Services.obs.notifyObservers(null, "metro_softkeyboard_hidden", null);

    let rect2 = text.getBoundingClientRect();
    is(rect2.top, rect0.top, "text field moves back to the original position");

    finish();
  }
});

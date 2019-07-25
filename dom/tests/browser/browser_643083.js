


Components.utils.import("resource://gre/modules/Services.jsm");

function test() {
  waitForExplicitFinish();

  let win = Services.ww.openWindow(
    window, "chrome://mochitests/content/browser/dom/tests/browser/test_643083.xul", "_blank", "chrome", {});
  win.addEventListener("load", function() {
    let browser = win.browser();
    browser.messageManager.addMessageListener("scroll", function fn(msg) {
      
      
      
      
      
      
      
      
      window.addEventListener("dummy-event", function() {
        win.close();

        setTimeout(function() {
          ok(true, "Completed message to close window");
          finish();
        }, 0);
      }, false);

      let e = document.createEvent("UIEvents");
      e.initUIEvent("dummy-event", true, false, window, 0);
      window.dispatchEvent(e);

      finish();
    });
  }, false);
}






const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");

function promiseBrowserEvent(browser, eventType) {
  return new Promise((resolve) => {
    function handle(event) {
      
      if (event.target != browser.contentDocument || event.target.location.href == "about:blank") {
        do_print("Skipping spurious '" + eventType + "' event" + " for " + event.target.location.href);
        return;
      }
      do_print("Received event " + eventType + " from browser");
      browser.removeEventListener(eventType, handle, true);
      resolve(event);
    }

    browser.addEventListener(eventType, handle, true);
    do_print("Now waiting for " + eventType + " event from browser");
  });
}

function promiseWaitForCondition(win, condition) {
  return new Promise((resolve, reject) => {
    var tries = 0;
    var interval = win.setInterval(function() {
      if (tries >= 30) {
        do_print("Condition didn't pass. Moving on.");
        moveOn();
      }
      if (condition()) {
        moveOn();
      }
      tries++;
    }, 200);
    var moveOn = function() { win.clearInterval(interval); resolve(); };
  });
}

add_task(function* test_reader_view_visibility() {
  let gWin = Services.wm.getMostRecentWindow("navigator:browser");
  let BrowserApp = gWin.BrowserApp;

  let url = "http://mochi.test:8888/tests/robocop/reader_mode_pages/basic_article.html";
  let browser = BrowserApp.addTab("about:reader?url=" + url).browser;

  yield promiseBrowserEvent(browser, "load");

  do_register_cleanup(function cleanup() {
    BrowserApp.closeTab(BrowserApp.getTabForBrowser(browser));
  });

  let doc = browser.contentDocument;
  let title = doc.getElementById("reader-title");

  
  
  yield promiseWaitForCondition(gWin, () => title.textContent);
  do_check_eq(title.textContent, "Article title");
});

run_next_test();

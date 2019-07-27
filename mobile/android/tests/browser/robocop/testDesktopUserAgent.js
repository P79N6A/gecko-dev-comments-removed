




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");

function ok(passed, text) {
  do_report_result(passed, text, Components.stack.caller, false);
}

function is(lhs, rhs, text) {
  do_report_result(lhs === rhs, text, Components.stack.caller, false);
}

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


const TestURI = Services.io.newURI("http://mochi.test:8888/tests/robocop/desktopmode_user_agent.sjs", null, null);

add_task(function* test_desktopmode() {
  let chromeWin = Services.wm.getMostRecentWindow("navigator:browser");
  let BrowserApp = chromeWin.BrowserApp;

  
  let desktopBrowser = BrowserApp.addTab(TestURI.spec, { selected: true, parentId: BrowserApp.selectedTab.id, desktopMode: true }).browser;
  yield promiseBrowserEvent(desktopBrowser, "load");

  
  do_print("desktop: " + desktopBrowser.contentWindow.navigator.userAgent);
  do_print("desktop: " + desktopBrowser.contentDocument.body.innerHTML);

  
  ok(desktopBrowser.contentWindow.navigator.userAgent.indexOf("Linux x86_64") != -1, "window.navigator.userAgent has 'Linux' in it");
  ok(desktopBrowser.contentDocument.body.innerHTML.indexOf("Linux x86_64") != -1, "HTTP header 'User-Agent' has 'Linux' in it");

  
  let mobileBrowser = BrowserApp.addTab(TestURI.spec, { selected: true, parentId: BrowserApp.selectedTab.id }).browser;
  yield promiseBrowserEvent(mobileBrowser, "load");

  
  do_print("mobile: " + mobileBrowser.contentWindow.navigator.userAgent);
  do_print("mobile: " + mobileBrowser.contentDocument.body.innerHTML);

  
  
  ok(mobileBrowser.contentWindow.navigator.userAgent.indexOf("Android") != -1, "window.navigator.userAgent has 'Android' in it");
  ok(mobileBrowser.contentDocument.body.innerHTML.indexOf("Android") != -1, "HTTP header 'User-Agent' has 'Android' in it");
});

run_next_test();




const TEST_VALUE = "example.com";
const START_VALUE = "example.org";

add_task(function* setup() {
  Services.prefs.setBoolPref("browser.altClickSave", true);

  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("browser.altClickSave");
  });
});

add_task(function* alt_left_click_test() {
  info("Running test: Alt left click");

  
  let oldSaveURL = saveURL;
  let saveURLPromise = new Promise(resolve => {
    saveURL = () => {
      
      saveURL = oldSaveURL;
      resolve();
    };
  });

  triggerCommand(true, {altKey: true});

  yield saveURLPromise;
  ok(true, "SaveURL was called");
  is(gURLBar.value, "", "Urlbar reverted to original value");
});

add_task(function* shift_left_click_test() {
  info("Running test: Shift left click");

  let newWindowPromise = promiseWaitForNewWindow();
  triggerCommand(true, {shiftKey: true});
  let win = yield newWindowPromise;

  
  let browser = win.gBrowser.selectedBrowser;
  yield BrowserTestUtils.browserLoaded(browser);

  info("URL should be loaded in a new window");
  is(gURLBar.value, "", "Urlbar reverted to original value");
  let childFocus = yield promiseCheckChildNoFocusedElement(gBrowser.selectedBrowser);
  ok(childFocus, "There should be no focused element");
  is(document.activeElement, gBrowser.selectedBrowser, "Content window should be focused");
  is(win.gURLBar.textValue, TEST_VALUE, "New URL is loaded in new window");

  
  yield promiseWindowClosed(win);
});

add_task(function* right_click_test() {
  info("Running test: Right click on go button");

  
  yield* promiseOpenNewTab();

  triggerCommand(true, {button: 2});

  
  is(gURLBar.value, TEST_VALUE, "Urlbar still has the value we entered");

  
  gBrowser.removeCurrentTab();
});

add_task(function* shift_accel_left_click_test() {
  info("Running test: Shift+Ctrl/Cmd left click on go button");

  
  let tab = yield* promiseOpenNewTab();

  let loadStartedPromise = promiseLoadStarted();
  triggerCommand(true, {accelKey: true, shiftKey: true});
  yield loadStartedPromise;

  
  info("URL should be loaded in a new background tab");
  is(gURLBar.value, "", "Urlbar reverted to original value");
  ok(!gURLBar.focused, "Urlbar is no longer focused after urlbar command");
  is(gBrowser.selectedTab, tab, "Focus did not change to the new tab");

  
  gBrowser.selectedTab = gBrowser.selectedTab.nextSibling;
  is(gURLBar.value, TEST_VALUE, "New URL is loaded in new tab");

  
  gBrowser.removeCurrentTab();
  gBrowser.removeCurrentTab();
});

add_task(function* load_in_current_tab_test() {
  let tests = [
    {desc: "Simple return keypress"},
    {desc: "Left click on go button", click: true},
    {desc: "Ctrl/Cmd+Return keypress", event: {accelKey: true}},
    {desc: "Alt+Return keypress in a blank tab", event: {altKey: true}}
  ];

  for (let test of tests) {
    info(`Running test: ${test.desc}`);

    
    let tab = yield* promiseOpenNewTab();

    
    let loadStartedPromise = promiseLoadStarted();
    triggerCommand(test.click || false, test.event || {});
    yield loadStartedPromise;

    info("URL should be loaded in the current tab");
    is(gURLBar.value, TEST_VALUE, "Urlbar still has the value we entered");
    let childFocus = yield promiseCheckChildNoFocusedElement(gBrowser.selectedBrowser);
    ok(childFocus, "There should be no focused element");
    is(document.activeElement, gBrowser.selectedBrowser, "Content window should be focused");
    is(gBrowser.selectedTab, tab, "New URL was loaded in the current tab");

    
    gBrowser.removeCurrentTab();
  }
});

add_task(function* load_in_new_tab_test() {
  let tests = [
    {desc: "Ctrl/Cmd left click on go button", click: true, event: {accelKey: true}},
    {desc: "Alt+Return keypress in a dirty tab", event: {altKey: true}, url: START_VALUE}
  ];

  for (let test of tests) {
    info(`Running test: ${test.desc}`);

    
    let tab = yield* promiseOpenNewTab(test.url || "about:blank");

    
    let tabSwitchedPromise = promiseNewTabSwitched();
    triggerCommand(test.click || false, test.event || {});
    yield tabSwitchedPromise;

    
    info("URL should be loaded in a new focused tab");
    is(gURLBar.inputField.value, TEST_VALUE, "Urlbar still has the value we entered");
    let childFocus = yield promiseCheckChildNoFocusedElement(gBrowser.selectedBrowser);
    ok(childFocus, "There should be no focused element");
    is(document.activeElement, gBrowser.selectedBrowser, "Content window should be focused");
    isnot(gBrowser.selectedTab, tab, "New URL was loaded in a new tab");

    
    gBrowser.removeCurrentTab();
    gBrowser.removeCurrentTab();
  }
});

function triggerCommand(shouldClick, event) {
  gURLBar.value = TEST_VALUE;
  gURLBar.focus();

  if (shouldClick) {
    is(gURLBar.getAttribute("pageproxystate"), "invalid",
       "page proxy state must be invalid for go button to be visible");

    let goButton = document.getElementById("urlbar-go-button");
    EventUtils.synthesizeMouseAtCenter(goButton, event);
  } else {
    EventUtils.synthesizeKey("VK_RETURN", event);
  }
}

function promiseLoadStarted() {
  return new Promise(resolve => {
    gBrowser.addTabsProgressListener({
      onStateChange(browser, webProgress, req, flags, status) {
        if (flags & Ci.nsIWebProgressListener.STATE_START) {
          gBrowser.removeTabsProgressListener(this);
          resolve();
        }
      }
    });
  });
}

function* promiseOpenNewTab(url = "about:blank") {
  let tab = gBrowser.addTab(url);
  let tabSwitchPromise = promiseNewTabSwitched(tab);
  gBrowser.selectedTab = tab;
  yield BrowserTestUtils.browserLoaded(gBrowser.selectedBrowser);
  yield tabSwitchPromise;
  return tab;
}

function promiseNewTabSwitched() {
  return new Promise(resolve => {
    gBrowser.addEventListener("TabSwitchDone", function onSwitch() {
      gBrowser.removeEventListener("TabSwitchDone", onSwitch);
      executeSoon(resolve);
    });
  });
}

function promiseWaitForNewWindow() {
  return new Promise(resolve => {
    let listener = {
      onOpenWindow(xulWindow) {
        let win = xulWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIDOMWindow);

        Services.wm.removeListener(listener);
        whenDelayedStartupFinished(win, () => resolve(win));
      },

      onCloseWindow() {},
      onWindowTitleChange() {}
    };

    Services.wm.addListener(listener);
  });
}

function promiseCheckChildNoFocusedElement(browser)
{
  if (!gMultiProcessBrowser) {
    return Services.focus.focusedElement == null;
  }

  return ContentTask.spawn(browser, { }, function* () {
    const fm = Components.classes["@mozilla.org/focus-manager;1"].
                          getService(Components.interfaces.nsIFocusManager);
    return fm.focusedElement == null;
  });
}

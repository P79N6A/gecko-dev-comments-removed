




"use strict";

function debugClipFlavors(aClip)
{
  let xfer = Cc["@mozilla.org/widget/transferable;1"].
             createInstance(Ci.nsITransferable);
  xfer.init(null);
  aClip.getData(xfer, Ci.nsIClipboard.kGlobalClipboard);
  let array = xfer.flavorsTransferableCanExport();
  let count = array.Count();
  info("flavors:" + count);
  for (let idx = 0; idx < count; idx++) {
    let string = array.GetElementAt(idx).QueryInterface(Ci.nsISupportsString);
    info("[" + idx + "] " + string);
  }
}

function checkContextMenuPositionRange(aElement, aMinLeft, aMaxLeft, aMinTop, aMaxTop) {
  ok(aElement.left > aMinLeft && aElement.left < aMaxLeft,
    "Left position is " + aElement.left + ", expected between " + aMinLeft + " and " + aMaxLeft);

  ok(aElement.top > aMinTop && aElement.top < aMaxTop,
    "Top position is " + aElement.top + ", expected between " + aMinTop + " and " + aMaxTop);
}

gTests.push({
  desc: "text context menu",
  run: function test() {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

    info(chromeRoot + "browser_context_menu_tests_02.html");
    yield addTab(chromeRoot + "browser_context_menu_tests_02.html");

    purgeEventQueue();
    emptyClipboard();

    let win = Browser.selectedTab.browser.contentWindow;

    yield hideContextUI();

    
    

    
    let span = win.document.getElementById("text1");
    win.getSelection().selectAllChildren(span);

    yield waitForMs(0);

    
    let promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, span, 85, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    
    checkContextUIMenuItemVisibility(["context-copy",
                                      "context-search"]);

    let menuItem = document.getElementById("context-copy");
    promise = waitForEvent(document, "popuphidden");
    EventUtils.synthesizeMouse(menuItem, 10, 10, {}, win);

    yield promise;

    
    let string = "";
    yield waitForCondition(function () {
      string = SpecialPowers.getClipboardData("text/unicode");
      return string === span.textContent;
    });
    ok(string === span.textContent, "copied selected text from span");

    win.getSelection().removeAllRanges();

    
    

    
    let link = win.document.getElementById("text2-link");
    win.getSelection().selectAllChildren(link);
    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, link, 40, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    
    checkContextUIMenuItemVisibility(["context-copy",
                                      "context-search",
                                      "context-open-in-new-tab",
                                      "context-copy-link"]);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;
    win.getSelection().removeAllRanges();

    
    

    link = win.document.getElementById("text2-link");
    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, link, 40, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    
    checkContextUIMenuItemVisibility(["context-open-in-new-tab",
                                      "context-copy-link",
                                      "context-bookmark-link"]);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;

    
    

    emptyClipboard();

    let input = win.document.getElementById("text3-input");
    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, input, 20, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    checkContextUIMenuItemVisibility(["context-select",
                                      "context-select-all"]);

    
    let menuItem = document.getElementById("context-copy");
    ok(menuItem && menuItem.hidden, "menu item is not visible");

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;

    
    

    let input = win.document.getElementById("text3-input");
    input.value = "hello, I'm sorry but I must be going.";
    input.setSelectionRange(0, 5);
    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, input, 20, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    checkContextUIMenuItemVisibility(["context-cut",
                                      "context-copy"]);

    let menuItem = document.getElementById("context-copy");
    let popupPromise = waitForEvent(document, "popuphidden");
    EventUtils.synthesizeMouse(menuItem, 10, 10, {}, win);

    yield popupPromise;

    
    let string = "";
    yield waitForCondition(function () {
      string = SpecialPowers.getClipboardData("text/unicode");
      return string === "hello";
    });

    ok(string === "hello", "copied selected text");

    emptyClipboard();

    
    

    input = win.document.getElementById("text3-input");
    input.select();
    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, input, 20, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    
    checkContextUIMenuItemVisibility(["context-cut",
                                      "context-copy"]);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;

    
    

    SpecialPowers.clipboardCopyString("foo");
    input = win.document.getElementById("text3-input");
    input.select();
    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, input, 20, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    
    checkContextUIMenuItemVisibility(["context-cut",
                                      "context-copy",
                                      "context-paste"]);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;

    
    

    emptyClipboard();

    let input = win.document.getElementById("text3-input");
    input.value = "hello, I'm sorry but I must be going.";
    input.setSelectionRange(0, 5);
    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, input, 20, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    checkContextUIMenuItemVisibility(["context-cut",
                                      "context-copy"]);

    let menuItem = document.getElementById("context-cut");
    let popupPromise = waitForEvent(document, "popuphidden");
    EventUtils.synthesizeMouse(menuItem, 10, 10, {}, win);

    yield popupPromise;

    
    let string = "";
    yield waitForCondition(function () {
      string = SpecialPowers.getClipboardData("text/unicode");
      return string === "hello";
    });

    let inputValue = input.value;
    ok(string === "hello", "cut selected text in clipboard");
    ok(inputValue === ", I'm sorry but I must be going.", "cut selected text from input value");

    emptyClipboard();

    
    

    SpecialPowers.clipboardCopyString("foo");
    input = win.document.getElementById("text3-input");
    input.value = "";

    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, input, 20, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    
    checkContextUIMenuItemVisibility(["context-paste"]);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;

    
    

    emptyClipboard();
    ContextUI.dismiss();

    input = win.document.getElementById("text3-input");
    input.value = "";

    promise = waitForEvent(Elements.tray, "transitionend");
    sendContextMenuClickToElement(win, input, 20, 10);
    yield promise;

    
    ok(!ContextMenuUI._menuPopup._visible, "is visible");

    
    yield hideContextUI();

    Browser.closeTab(Browser.selectedTab, { forceClose: true });
    purgeEventQueue();
  }
});

gTests.push({
  desc: "checks for context menu positioning when browser shifts",
  run: function test() {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

    info(chromeRoot + "browser_context_menu_tests_02.html");
    yield addTab(chromeRoot + "browser_context_menu_tests_02.html");

    purgeEventQueue();
    emptyClipboard();

    let browserwin = Browser.selectedTab.browser.contentWindow;

    yield hideContextUI();

    
    
    

    yield showNotification();

    
    let span = browserwin.document.getElementById("text4");
    browserwin.getSelection().selectAllChildren(span);

    
    let promise = waitForEvent(document, "popupshown");
    sendContextMenuClick(225, 310);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    let notificationBox = Browser.getNotificationBox();
    let notification = notificationBox.getNotificationWithValue("popup-blocked");
    let notificationHeight = notification.boxObject.height;

    checkContextMenuPositionRange(ContextMenuUI._panel, 65, 80, notificationHeight +  155, notificationHeight + 180);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;

    Browser.closeTab(Browser.selectedTab, { forceClose: true });
  }
});

var observeLogger = {
  observe: function (aSubject, aTopic, aData) {
    info("observeLogger: " + aTopic);
  },
  QueryInterface: function (aIID) {
    if (!aIID.equals(Ci.nsIObserver) &&
        !aIID.equals(Ci.nsISupportsWeakReference) &&
        !aIID.equals(Ci.nsISupports)) {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },
  init: function init() {
    Services.obs.addObserver(observeLogger, "dl-start", true);
    Services.obs.addObserver(observeLogger, "dl-done", true);
    Services.obs.addObserver(observeLogger, "dl-failed", true);
    Services.obs.addObserver(observeLogger, "dl-scanning", true);
    Services.obs.addObserver(observeLogger, "dl-blocked", true);
    Services.obs.addObserver(observeLogger, "dl-dirty", true);
    Services.obs.addObserver(observeLogger, "dl-cancel", true);
  },
  shutdown: function shutdown() {
    Services.obs.removeObserver(observeLogger, "dl-start");
    Services.obs.removeObserver(observeLogger, "dl-done");
    Services.obs.removeObserver(observeLogger, "dl-failed");
    Services.obs.removeObserver(observeLogger, "dl-scanning");
    Services.obs.removeObserver(observeLogger, "dl-blocked");
    Services.obs.removeObserver(observeLogger, "dl-dirty");
    Services.obs.removeObserver(observeLogger, "dl-cancel");
  }
}


gTests.push({
  desc: "image context menu",
  setUp: function() {
    observeLogger.init();
  },
  tearDown: function() {
    observeLogger.shutdown();
  },
  run: function test() {
    info(chromeRoot + "browser_context_menu_tests_01.html");
    yield addTab(chromeRoot + "browser_context_menu_tests_01.html");

    let win = Browser.selectedTab.browser.contentWindow;

    purgeEventQueue();

    yield hideContextUI();

    
    
    yield waitForImageLoad(win, "image01");

    
    
    

















































    
    

    let promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToWindow(win, 20, 20);
    yield promise;
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    let menuItem = document.getElementById("context-copy-image");
    ok(menuItem, "menu item exists");
    ok(!menuItem.hidden, "menu item visible");
    let popupPromise = waitForEvent(document, "popuphidden");
    EventUtils.synthesizeMouse(menuItem, 10, 10, {}, win);
    yield popupPromise;

    purgeEventQueue();

    let clip = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
    let flavors = ["image/png"];
    ok(clip.hasDataMatchingFlavors(flavors, flavors.length, Ci.nsIClipboard.kGlobalClipboard), "clip has my png flavor");

    
    

    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToWindow(win, 30, 30);
    yield promise;
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    menuItem = document.getElementById("context-copy-image-loc");
    ok(menuItem, "menu item exists");
    ok(!menuItem.hidden, "menu item visible");
    popupPromise = waitForEvent(document, "popuphidden");
    EventUtils.synthesizeMouse(menuItem, 10, 10, {}, win);
    yield popupPromise;

    purgeEventQueue();

    let clip = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
    let flavors = ["text/unicode"];
    ok(clip.hasDataMatchingFlavors(flavors, flavors.length, Ci.nsIClipboard.kGlobalClipboard), "clip has my text flavor");

    let xfer = Cc["@mozilla.org/widget/transferable;1"].
               createInstance(Ci.nsITransferable);
    xfer.init(null);
    xfer.addDataFlavor("text/unicode");
    clip.getData(xfer, Ci.nsIClipboard.kGlobalClipboard);
    let str = new Object();
    let strLength = new Object();
    xfer.getTransferData("text/unicode", str, strLength);
    str = str.value.QueryInterface(Components.interfaces.nsISupportsString);

    ok(str == chromeRoot + "res/image01.png", "url copied");

    
    

    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToWindow(win, 40, 40);
    yield promise;
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    menuItem = document.getElementById("context-open-image-tab");
    ok(menuItem, "menu item exists");
    ok(!menuItem.hidden, "menu item visible");
    let tabPromise = waitForEvent(document, "TabOpen");
    popupPromise = waitForEvent(document, "popuphidden");
    EventUtils.synthesizeMouse(menuItem, 10, 10, {}, win);
    yield popupPromise;
    let event = yield tabPromise;

    purgeEventQueue();

    let imagetab = Browser.getTabFromChrome(event.originalTarget);
    ok(imagetab != null, "tab created");

    Browser.closeTab(imagetab, { forceClose: true });
  }
});

gTests.push({
  desc: "tests for subframe positioning",
  run: function test() {
    info(chromeRoot + "browser_context_menu_tests_03.html");
    yield addTab(chromeRoot + "browser_context_menu_tests_03.html");

    let win = Browser.selectedTab.browser.contentWindow;

    
    try {
      yield waitForCondition(function () {
        return ContextUI.isVisible;
      }, 500, 50);
    } catch (ex) {}

    ContextUI.dismiss();

    let frame1 = win.document.getElementById("frame1");
    let link1 = frame1.contentDocument.getElementById("link1");

    let promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(frame1.contentDocument.defaultView, link1, 85, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    checkContextMenuPositionRange(ContextMenuUI._panel, 265, 280, 175, 190);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;

    frame1.contentDocument.defaultView.scrollBy(0, 200);

    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(frame1.contentDocument.defaultView, link1, 85, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    checkContextMenuPositionRange(ContextMenuUI._panel, 265, 280, 95, 110);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;

    let rlink1 = win.document.getElementById("rlink1");

    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, rlink1, 40, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    checkContextMenuPositionRange(ContextMenuUI._panel, 295, 310, 540, 555);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;

    win.scrollBy(0, 200);

    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(win, rlink1, 40, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    checkContextMenuPositionRange(ContextMenuUI._panel, 295, 310, 340, 355);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;

    let link2 = frame1.contentDocument.getElementById("link2");

    promise = waitForEvent(document, "popupshown");
    sendContextMenuClickToElement(frame1.contentDocument.defaultView, link2, 85, 10);
    yield promise;

    
    ok(ContextMenuUI._menuPopup._visible, "is visible");

    checkContextMenuPositionRange(ContextMenuUI._panel, 265, 280, 110, 125);

    promise = waitForEvent(document, "popuphidden");
    ContextMenuUI.hide();
    yield promise;
  }
});

function test() {
  runTests();
}

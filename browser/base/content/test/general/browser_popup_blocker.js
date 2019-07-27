



const baseURL = "http://example.com/browser/browser/base/content/test/general/";

function test() {
  waitForExplicitFinish();
  Task.spawn(function* () {
    
    yield pushPrefs(["dom.disable_open_during_load", true]);

    
    let tab = gBrowser.loadOneTab(baseURL + "popup_blocker.html", { inBackground: false });
    yield promiseTabLoaded(tab);

    
    let notification;
    yield promiseWaitForCondition(() =>
      notification = gBrowser.getNotificationBox().getNotificationWithValue("popup-blocked"));

    
    let popupShown = promiseWaitForEvent(window, "popupshown");
    notification.querySelector("button").doCommand();
    let popup_event = yield popupShown;
    let menu = popup_event.target;
    is(menu.id, "blockedPopupOptions", "Blocked popup menu shown");

    
    let sep = menu.querySelector("menuseparator");
    let popupCount = 0;
    for (let i = sep.nextElementSibling; i; i = i.nextElementSibling) {
      popupCount++;
    }
    is(popupCount, 2, "Two popups were blocked");

    
    let popupTabs = [];
    function onTabOpen(event) {
      popupTabs.push(event.target);
    }
    gBrowser.tabContainer.addEventListener("TabOpen", onTabOpen);

    
    let allow = menu.querySelector("[observes='blockedPopupAllowSite']");
    allow.doCommand();
    yield promiseWaitForCondition(() =>
      popupTabs.length == 2 &&
      popupTabs.every(tab => tab.linkedBrowser.currentURI.spec != "about:blank"));

    gBrowser.tabContainer.removeEventListener("TabOpen", onTabOpen);

    is(popupTabs[0].linkedBrowser.currentURI.spec, "data:text/plain;charset=utf-8,a", "Popup a");
    is(popupTabs[1].linkedBrowser.currentURI.spec, "data:text/plain;charset=utf-8,b", "Popup b");

    
    gBrowser.removeTab(tab);
    for (let popup of popupTabs) {
      gBrowser.removeTab(popup);
    }
    clearAllPermissionsByPrefix("popup");
    finish();
  });
}

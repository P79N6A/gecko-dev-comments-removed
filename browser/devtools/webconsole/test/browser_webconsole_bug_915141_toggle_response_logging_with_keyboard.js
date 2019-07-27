








"use strict";

const TEST_URI = "data:text/html;charset=utf-8,Web Console test for " +
                 "bug 915141: Toggle log response bodies with keyboard";
let hud;

function test() {
  let saveBodiesMenuItem;
  let saveBodiesContextMenuItem;

  loadTab(TEST_URI).then(({tab: tab}) => {
    return openConsole(tab);
  })
  .then((aHud) => {
    hud = aHud;
    saveBodiesMenuItem = hud.ui.rootElement.querySelector("#saveBodies");
    saveBodiesContextMenuItem = hud.ui.rootElement.querySelector("#saveBodiesContextMenu");

    
    info("Testing 'Log Request and Response Bodies' menuitem of right click " +
         "context menu.");

    return openPopup(saveBodiesContextMenuItem);
  })
  .then(() => {
    is(saveBodiesContextMenuItem.getAttribute("checked"), "false",
       "Context menu: 'log responses' is not checked before action.");
    is(hud.ui._saveRequestAndResponseBodies, false,
       "Context menu: Responses are not logged before action.");

    EventUtils.synthesizeKey("VK_DOWN", {});
    EventUtils.synthesizeKey("VK_RETURN", {});

    return waitForUpdate(saveBodiesContextMenuItem);
  })
  .then(() => {
    is(saveBodiesContextMenuItem.getAttribute("checked"), "true",
       "Context menu: 'log responses' is checked after menuitem was selected " +
       "with keyboard.");
    is(hud.ui._saveRequestAndResponseBodies, true,
       "Context menu: Responses are saved after menuitem was selected with " +
       "keyboard.");

    return openPopup(saveBodiesMenuItem);
  })
  .then(() => {
    
    info("Testing 'Log Request and Response Bodies' menuitem of 'Net' menu " +
         "in the console.");
    
    

    is(saveBodiesMenuItem.getAttribute("checked"), "true",
       "Console net menu: 'log responses' is checked before action.");
    is(hud.ui._saveRequestAndResponseBodies, true,
       "Console net menu: Responses are logged before action.");

    
    EventUtils.synthesizeKey("VK_UP", {});
    EventUtils.synthesizeKey("VK_RETURN", {});

    return waitForUpdate(saveBodiesMenuItem);
  })
  .then(() => {
    is(saveBodiesMenuItem.getAttribute("checked"), "false",
       "Console net menu: 'log responses' is NOT checked after menuitem was " +
       "selected with keyboard.");
    is(hud.ui._saveRequestAndResponseBodies, false,
       "Responses are NOT saved after menuitem was selected with keyboard.");
    hud = null;
  })
  .then(finishTest);
}







function openPopup(menuItem) {
  let menu = menuItem.parentNode;

  let menuOpened = promise.defer();
  let uiUpdated = promise.defer();
  
  
  
  
  hud.ui.once("save-bodies-ui-toggled", uiUpdated.resolve);
  menu.addEventListener("popupshown", function onPopup() {
    menu.removeEventListener("popupshown", onPopup);
    menuOpened.resolve();
  });

  menu.openPopup();
  return Promise.all([menuOpened.promise, uiUpdated.promise]);
}







function waitForUpdate(menuItem) {
  info("Waiting for settings update to complete.");
  let deferred = promise.defer();
  hud.ui.once("save-bodies-pref-reversed", function() {
    hud.ui.once("save-bodies-ui-toggled", deferred.resolve);
    
    menuItem.parentNode.openPopup();
  });
  return deferred.promise;
}

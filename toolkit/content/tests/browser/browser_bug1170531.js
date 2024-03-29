


add_task(function* () {
  
  let winUtils = window.QueryInterface(Ci.nsIInterfaceRequestor).
        getInterface(Ci.nsIDOMWindowUtils);

  let editMenu = document.getElementById("edit-menu");
  let menubar = editMenu.parentNode;
  let menuPopup = editMenu.menupopup;
  let editMenuIndex = -1;
  for (let i = 0; i < menubar.children.length; i++) {
    if (menubar.children[i] === editMenu) {
      editMenuIndex = i;
      break;
    }
  }

  let closeMenu = function(aCallback) {
    if (OS.Constants.Sys.Name == "Darwin") {
      executeSoon(aCallback);
      return;
    }

    menuPopup.addEventListener("popuphidden", function onPopupHidden() {
      menuPopup.removeEventListener("popuphidden", onPopupHidden, false);
      executeSoon(aCallback);
    }, false);

    executeSoon(function() {
      editMenu.open = false;
    });
  };

  let openMenu = function(aCallback) {
    if (OS.Constants.Sys.Name == "Darwin") {
      goUpdateGlobalEditMenuItems();
      
      
      
      setTimeout(aCallback, 1000);
      return;
    }

    menuPopup.addEventListener("popupshown", function onPopupShown() {
      menuPopup.removeEventListener("popupshown", onPopupShown, false);
      executeSoon(aCallback);
    }, false);

    executeSoon(function() {
      editMenu.open = true;
    });
  };

  yield BrowserTestUtils.withNewTab({ gBrowser: gBrowser, url: "about:blank" }, function* (browser) {
    let menu_cut_disabled, menu_copy_disabled;

    yield BrowserTestUtils.loadURI(browser, "data:text/html,<div>hello!</div>");
    browser.focus();
    yield new Promise(resolve => waitForFocus(resolve, window));
    yield new Promise(openMenu);
    menu_cut_disabled = menuPopup.querySelector("#menu_cut").getAttribute('disabled') == "true";
    is(menu_cut_disabled, false, "menu_cut should be enabled");
    menu_copy_disabled = menuPopup.querySelector("#menu_copy").getAttribute('disabled') == "true";
    is(menu_copy_disabled, false, "menu_copy should be enabled");
    yield new Promise(closeMenu);

    yield BrowserTestUtils.loadURI(browser, "data:text/html,<div contentEditable='true'>hello!</div>");
    browser.focus();
    yield new Promise(resolve => waitForFocus(resolve, window));
    yield new Promise(openMenu);
    menu_cut_disabled = menuPopup.querySelector("#menu_cut").getAttribute('disabled') == "true";
    is(menu_cut_disabled, false, "menu_cut should be enabled");
    menu_copy_disabled = menuPopup.querySelector("#menu_copy").getAttribute('disabled') == "true";
    is(menu_copy_disabled, false, "menu_copy should be enabled");
    yield new Promise(closeMenu);

    yield BrowserTestUtils.loadURI(browser, "about:preferences");
    browser.focus();
    yield new Promise(resolve => waitForFocus(resolve, window));
    yield new Promise(openMenu);
    menu_cut_disabled = menuPopup.querySelector("#menu_cut").getAttribute('disabled') == "true";
    is(menu_cut_disabled, true, "menu_cut should be disabled");
    menu_copy_disabled = menuPopup.querySelector("#menu_copy").getAttribute('disabled') == "true";
    is(menu_copy_disabled, true, "menu_copy should be disabled");
    yield new Promise(closeMenu);
  });
});

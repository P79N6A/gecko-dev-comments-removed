


const searchbar = document.getElementById("searchbar");
const searchIcon = document.getAnonymousElementByAttribute(searchbar, "anonid", "searchbar-search-button");
const textbox = searchbar._textbox;
const searchPopup = document.getElementById("PopupSearchAutoComplete");

function promiseNewEngine(basename) {
  return new Promise((resolve, reject) => {
    info("Waiting for engine to be added: " + basename);
    Services.search.init({
      onInitComplete: function() {
        let url = getRootDirectory(gTestPath) + basename;
        let current = Services.search.currentEngine;
        Services.search.addEngine(url, Ci.nsISearchEngine.TYPE_MOZSEARCH, "", false, {
          onSuccess: function (engine) {
            info("Search engine added: " + basename);
            Services.search.currentEngine = engine;
            registerCleanupFunction(() => {
              Services.search.currentEngine = current;
              Services.search.removeEngine(engine);
              info("Search engine removed: " + basename);
            });
            resolve(engine);
          },
          onError: function (errCode) {
            ok(false, "addEngine failed with error code " + errCode);
            reject();
          }
        });
      }
    });
  });
}

add_task(function* init() {
  yield promiseNewEngine("testEngine.xml");
});


function add_no_popup_task(task) {
  add_task(function*() {
    let sawPopup = false;
    function listener() {
      sawPopup = true;
    }

    info("Entering test " + task.name);
    searchPopup.addEventListener("popupshowing", listener, false);
    yield Task.spawn(task);
    searchPopup.removeEventListener("popupshowing", listener, false);
    ok(!sawPopup, "Shouldn't have seen the suggestions popup");
    info("Leaving test " + task.name);
  });
}


function context_click(target) {
  for (let event of ["mousedown", "contextmenu", "mouseup"])
    EventUtils.synthesizeMouseAtCenter(target, { type: event, button: 2 });
}


add_no_popup_task(function* open_icon_context() {
  gURLBar.focus();
  let toolbarPopup = document.getElementById("toolbar-context-menu");

  let promise = promiseEvent(toolbarPopup, "popupshown");
  context_click(searchIcon);
  yield promise;

  promise = promiseEvent(toolbarPopup, "popuphidden");
  toolbarPopup.hidePopup();
  yield promise;
});


add_task(function* open_empty() {
  gURLBar.focus();

  let promise = promiseEvent(searchPopup, "popupshown");
  info("Clicking icon");
  EventUtils.synthesizeMouseAtCenter(searchIcon, {});
  yield promise;
  is(searchPopup.getAttribute("showonlysettings"), "true", "Should only show the settings");

  promise = promiseEvent(searchPopup, "popuphidden");
  info("Hiding popup");
  searchPopup.hidePopup();
  yield promise;
});


add_no_popup_task(function* click_doesnt_open_popup() {
  gURLBar.focus();

  EventUtils.synthesizeMouseAtCenter(textbox, {});
  is(Services.focus.focusedElement, textbox.inputField, "Should have focused the search bar");
  is(textbox.selectionStart, 0, "Should have selected all of the text");
  is(textbox.selectionEnd, 0, "Should have selected all of the text");
});


add_task(function* click_opens_popup() {
  gURLBar.focus();
  textbox.value = "foo";

  let promise = promiseEvent(searchPopup, "popupshown");
  EventUtils.synthesizeMouseAtCenter(textbox, {});
  yield promise;
  isnot(searchPopup.getAttribute("showonlysettings"), "true", "Should show the full popup");

  is(Services.focus.focusedElement, textbox.inputField, "Should have focused the search bar");
  is(textbox.selectionStart, 0, "Should have selected all of the text");
  is(textbox.selectionEnd, 3, "Should have selected all of the text");

  promise = promiseEvent(searchPopup, "popuphidden");
  searchPopup.hidePopup();
  yield promise;

  textbox.value = "";
});


add_no_popup_task(function* right_click_doesnt_open_popup() {
  gURLBar.focus();
  textbox.value = "foo";

  let contextPopup = document.getAnonymousElementByAttribute(textbox.inputField.parentNode, "anonid", "input-box-contextmenu");
  let promise = promiseEvent(contextPopup, "popupshown");
  context_click(textbox);
  yield promise;

  is(Services.focus.focusedElement, textbox.inputField, "Should have focused the search bar");
  is(textbox.selectionStart, 0, "Should have selected all of the text");
  is(textbox.selectionEnd, 3, "Should have selected all of the text");

  promise = promiseEvent(contextPopup, "popuphidden");
  contextPopup.hidePopup();
  yield promise;

  textbox.value = "";
});


add_task(function* focus_change_closes_popup() {
  gURLBar.focus();
  textbox.value = "foo";

  let promise = promiseEvent(searchPopup, "popupshown");
  EventUtils.synthesizeMouseAtCenter(textbox, {});
  yield promise;
  isnot(searchPopup.getAttribute("showonlysettings"), "true", "Should show the full popup");

  is(Services.focus.focusedElement, textbox.inputField, "Should have focused the search bar");
  is(textbox.selectionStart, 0, "Should have selected all of the text");
  is(textbox.selectionEnd, 3, "Should have selected all of the text");

  promise = promiseEvent(searchPopup, "popuphidden");
  let promise2 = promiseEvent(searchbar, "blur");
  EventUtils.synthesizeKey("VK_TAB", { shiftKey: true });
  yield promise;
  yield promise2;

  textbox.value = "";
});


add_task(function* escape_closes_popup() {
  gURLBar.focus();
  textbox.value = "foo";

  let promise = promiseEvent(searchPopup, "popupshown");
  EventUtils.synthesizeMouseAtCenter(textbox, {});
  yield promise;
  isnot(searchPopup.getAttribute("showonlysettings"), "true", "Should show the full popup");

  is(Services.focus.focusedElement, textbox.inputField, "Should have focused the search bar");
  is(textbox.selectionStart, 0, "Should have selected all of the text");
  is(textbox.selectionEnd, 3, "Should have selected all of the text");

  promise = promiseEvent(searchPopup, "popuphidden");
  EventUtils.synthesizeKey("VK_ESCAPE", {});
  yield promise;

  textbox.value = "";
});


add_task(function* tab_opens_popup() {
  gURLBar.focus();
  textbox.value = "foo";

  let promise = promiseEvent(searchPopup, "popupshown");
  EventUtils.synthesizeKey("VK_TAB", {});
  yield promise;
  isnot(searchPopup.getAttribute("showonlysettings"), "true", "Should show the full popup");

  is(Services.focus.focusedElement, textbox.inputField, "Should have focused the search bar");
  is(textbox.selectionStart, 0, "Should have selected all of the text");
  is(textbox.selectionEnd, 3, "Should have selected all of the text");

  promise = promiseEvent(searchPopup, "popuphidden");
  searchPopup.hidePopup();
  yield promise;

  textbox.value = "";
});


add_no_popup_task(function* tab_doesnt_open_popup() {
  gURLBar.focus();
  textbox.value = "foo";

  EventUtils.synthesizeKey("VK_TAB", {});

  is(Services.focus.focusedElement, textbox.inputField, "Should have focused the search bar");
  is(textbox.selectionStart, 0, "Should have selected all of the text");
  is(textbox.selectionEnd, 3, "Should have selected all of the text");

  textbox.value = "";
});


add_task(function* dont_consume_clicks() {
  gURLBar.focus();
  textbox.value = "foo";

  let promise = promiseEvent(searchPopup, "popupshown");
  EventUtils.synthesizeMouseAtCenter(textbox, {});
  yield promise;
  isnot(searchPopup.getAttribute("showonlysettings"), "true", "Should show the full popup");

  is(Services.focus.focusedElement, textbox.inputField, "Should have focused the search bar");
  is(textbox.selectionStart, 0, "Should have selected all of the text");
  is(textbox.selectionEnd, 3, "Should have selected all of the text");

  promise = promiseEvent(searchPopup, "popuphidden");
  EventUtils.synthesizeMouseAtCenter(gURLBar, {});
  yield promise;

  is(Services.focus.focusedElement, gURLBar.inputField, "Should have focused the URL bar");

  textbox.value = "";
});


add_task(function* refocus_window_doesnt_open_popup_mouse() {
  gURLBar.focus();
  textbox.value = "foo";

  let promise = promiseEvent(searchPopup, "popupshown");
  EventUtils.synthesizeMouseAtCenter(searchbar, {});
  yield promise;
  isnot(searchPopup.getAttribute("showonlysettings"), "true", "Should show the full popup");

  is(Services.focus.focusedElement, textbox.inputField, "Should have focused the search bar");
  is(textbox.selectionStart, 0, "Should have selected all of the text");
  is(textbox.selectionEnd, 3, "Should have selected all of the text");

  promise = promiseEvent(searchPopup, "popuphidden");
  let newWin = OpenBrowserWindow();
  yield new Promise(resolve => waitForFocus(resolve, newWin));
  yield promise;

  function listener() {
    ok(false, "Should not have shown the popup.");
  }
  searchPopup.addEventListener("popupshowing", listener, false);

  promise = promiseEvent(searchbar, "focus");
  newWin.close();
  yield promise;

  
  yield new Promise(resolve => executeSoon(resolve));
  yield new Promise(resolve => executeSoon(resolve));
  yield new Promise(resolve => executeSoon(resolve));

  searchPopup.removeEventListener("popupshowing", listener, false);
  textbox.value = "";
});


add_task(function* refocus_window_doesnt_open_popup_keyboard() {
  gURLBar.focus();
  textbox.value = "foo";

  let promise = promiseEvent(searchPopup, "popupshown");
  EventUtils.synthesizeKey("VK_TAB", {});
  yield promise;
  isnot(searchPopup.getAttribute("showonlysettings"), "true", "Should show the full popup");

  is(Services.focus.focusedElement, textbox.inputField, "Should have focused the search bar");
  is(textbox.selectionStart, 0, "Should have selected all of the text");
  is(textbox.selectionEnd, 3, "Should have selected all of the text");

  promise = promiseEvent(searchPopup, "popuphidden");
  let newWin = OpenBrowserWindow();
  yield new Promise(resolve => waitForFocus(resolve, newWin));
  yield promise;

  function listener() {
    ok(false, "Should not have shown the popup.");
  }
  searchPopup.addEventListener("popupshowing", listener, false);

  promise = promiseEvent(searchbar, "focus");
  newWin.close();
  yield promise;

  
  yield new Promise(resolve => executeSoon(resolve));
  yield new Promise(resolve => executeSoon(resolve));
  yield new Promise(resolve => executeSoon(resolve));

  searchPopup.removeEventListener("popupshowing", listener, false);
  textbox.value = "";
});

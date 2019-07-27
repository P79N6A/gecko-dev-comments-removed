

const searchbar = document.getElementById("searchbar");
const textbox = searchbar._textbox;
const searchPopup = document.getElementById("PopupSearchAutoComplete");
const searchIcon = document.getAnonymousElementByAttribute(searchbar, "anonid",
                                                           "searchbar-search-button");

const kValues = ["foo1", "foo2", "foo3"];


function getOneOffs() {
  let oneOffs = [];
  let oneOff = document.getAnonymousElementByAttribute(searchPopup, "anonid",
                                                       "search-panel-one-offs");
  for (oneOff = oneOff.firstChild; oneOff; oneOff = oneOff.nextSibling) {
    if (oneOff.classList.contains("dummy"))
      break;
    oneOffs.push(oneOff);
  }

  return oneOffs;
}

function getOpenSearchItems() {
  let os = [];

  let addEngineList =
    document.getAnonymousElementByAttribute(searchPopup, "anonid",
                                            "add-engines");
  for (let item = addEngineList.firstChild; item; item = item.nextSibling)
    os.push(item);

  return os;
}

add_task(function* init() {
  yield promiseNewEngine("testEngine.xml");

  
  yield new Promise((resolve, reject) => {
    info("cleanup the search history");
    searchbar.FormHistory.update({op: "remove", fieldname: "searchbar-history"},
                                 {handleCompletion: resolve,
                                  handleError: reject});
  });

  yield new Promise((resolve, reject) => {
    info("adding search history values: " + kValues);
    let ops = kValues.map(value => { return {op: "add",
                                             fieldname: "searchbar-history",
                                             value: value}
                                   });
    searchbar.FormHistory.update(ops, {
      handleCompletion: function() {
        registerCleanupFunction(() => {
          info("removing search history values: " + kValues);
          let ops =
            kValues.map(value => { return {op: "remove",
                                           fieldname: "searchbar-history",
                                           value: value}
                                 });
          searchbar.FormHistory.update(ops);
        });
        resolve();
      },
      handleError: reject
    });
  });
});


add_task(function* test_arrows() {
  let promise = promiseEvent(searchPopup, "popupshown");
  info("Opening search panel");
  EventUtils.synthesizeMouseAtCenter(searchIcon, {});
  yield promise;
info("textbox.mController.searchString = " + textbox.mController.searchString);
  is(textbox.mController.searchString, "", "The search string should be empty");

  
  is(searchPopup.getAttribute("showonlysettings"), "true", "Should show the small popup");
  
  
  is(searchPopup.view.rowCount, kValues.length, "There should be 3 suggestions");
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");

  
  
  
  let oneOffs = getOneOffs();
  ok(oneOffs.length >= 4, "we have at least 4 one-off buttons displayed")

  ok(!textbox.selectedButton, "no one-off button should be selected");

  
  EventUtils.synthesizeKey("VK_DOWN", {});
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  is(textbox.value, "", "the textfield value should be unmodified");

  
  for (let i = 0; i < oneOffs.length; ++i) {
    is(textbox.selectedButton, oneOffs[i],
       "the one-off button #" + (i + 1) + " should be selected");
    EventUtils.synthesizeKey("VK_DOWN", {});
  }

  is(textbox.selectedButton.getAttribute("anonid"), "search-settings",
     "the settings item should be selected");
  EventUtils.synthesizeKey("VK_DOWN", {});

  
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  ok(!textbox.selectedButton, "no one-off button should be selected");

  info("now test the up arrow key");
  EventUtils.synthesizeKey("VK_UP", {});
  is(textbox.selectedButton.getAttribute("anonid"), "search-settings",
     "the settings item should be selected");

  
  for (let i = oneOffs.length; i; --i) {
    EventUtils.synthesizeKey("VK_UP", {});
    is(textbox.selectedButton, oneOffs[i - 1],
       "the one-off button #" + i + " should be selected");
  }

  
  EventUtils.synthesizeKey("VK_UP", {});
  ok(!textbox.selectedButton, "no one-off button should be selected");
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  is(textbox.value, "", "the textfield value should be unmodified");
});

add_task(function* test_tab() {
  is(Services.focus.focusedElement, textbox.inputField,
     "the search bar should be focused"); 

  let oneOffs = getOneOffs();
  ok(!textbox.selectedButton, "no one-off button should be selected");

  
  
  for (let i = 0; i < oneOffs.length; ++i) {
    EventUtils.synthesizeKey("VK_TAB", {});
    is(textbox.selectedButton, oneOffs[i],
       "the one-off button #" + (i + 1) + " should be selected");
  }
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  is(textbox.value, "", "the textfield value should be unmodified");

  
  EventUtils.synthesizeKey("VK_TAB", {});
  is(textbox.selectedButton.getAttribute("anonid"), "search-settings",
     "the settings item should be selected");

  
  let promise = promiseEvent(searchPopup, "popuphidden");
  EventUtils.synthesizeKey("VK_TAB", {});
  yield promise;

  
  isnot(Services.focus.focusedElement, textbox.inputField,
        "the search bar no longer be focused");
});

add_task(function* test_shift_tab() {
  
  let promise = promiseEvent(searchPopup, "popupshown");
  info("Opening search panel");
  SimpleTest.executeSoon(() => {
    EventUtils.synthesizeMouseAtCenter(searchIcon, {});
  });
  yield promise;

  let oneOffs = getOneOffs();
  ok(!textbox.selectedButton, "no one-off button should be selected");
  is(searchPopup.getAttribute("showonlysettings"), "true", "Should show the small popup");

  
  EventUtils.synthesizeKey("VK_UP", {});
  is(textbox.selectedButton.getAttribute("anonid"), "search-settings",
     "the settings item should be selected");

  
  EventUtils.synthesizeKey("VK_UP", {});

  
  for (let i = oneOffs.length - 1; i >= 0; --i) {
    is(textbox.selectedButton, oneOffs[i],
       "the one-off button #" + (i + 1) + " should be selected");
    if (i)
      EventUtils.synthesizeKey("VK_TAB", {shiftKey: true});
  }
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  is(textbox.value, "", "the textfield value should be unmodified");

  
  promise = promiseEvent(searchPopup, "popuphidden");
  EventUtils.synthesizeKey("VK_TAB", {shiftKey: true});
  yield promise;

  
  isnot(Services.focus.focusedElement, textbox.inputField,
        "the search bar no longer be focused");
});

add_task(function* test_alt_down() {
  
  let promise = promiseEvent(searchPopup, "popupshown");
  info("Opening search panel");
  SimpleTest.executeSoon(() => {
    EventUtils.synthesizeMouseAtCenter(searchIcon, {});
  });
  yield promise;

  
  is(searchPopup.getAttribute("showonlysettings"), "true", "Should show the small popup");
  ok(!textbox.selectedButton, "no one-off button should be selected");
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  is(textbox.value, "", "the textfield value should be unmodified");

  
  
  let oneOffs = getOneOffs();
  for (let i = 0; i < oneOffs.length; ++i) {
    EventUtils.synthesizeKey("VK_DOWN", {altKey: true});
    is(textbox.selectedButton, oneOffs[i],
       "the one-off button #" + (i + 1) + " should be selected");
    is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  }

  
  EventUtils.synthesizeKey("VK_DOWN", {altKey: true});
  ok(!textbox.selectedButton, "no one-off button should be selected");

  
  EventUtils.synthesizeKey("VK_DOWN", {altKey: true});
  is(textbox.selectedButton, oneOffs[0],
     "the first one-off button should be selected");

  
  EventUtils.synthesizeKey("VK_UP", {altKey: true});
  ok(!textbox.selectedButton, "no one-off button should be selected");
});

add_task(function* test_alt_up() {
  
  ok(!textbox.selectedButton, "no one-off button should be selected");
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  is(textbox.value, "", "the textfield value should be unmodified");

  
  
  let oneOffs = getOneOffs();
  for (let i = oneOffs.length - 1; i >= 0; --i) {
    EventUtils.synthesizeKey("VK_UP", {altKey: true});
    is(textbox.selectedButton, oneOffs[i],
       "the one-off button #" + (i + 1) + " should be selected");
    is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  }

  
  EventUtils.synthesizeKey("VK_UP", {altKey: true});
  ok(!textbox.selectedButton, "no one-off button should be selected");

  
  EventUtils.synthesizeKey("VK_UP", {altKey: true});
  is(textbox.selectedButton, oneOffs[oneOffs.length - 1],
     "the last one-off button should be selected");

  
  EventUtils.synthesizeKey("VK_DOWN", {});
  is(textbox.selectedButton.getAttribute("anonid"), "search-settings",
     "the settings item should be selected");
  EventUtils.synthesizeKey("VK_DOWN", {});
  ok(!textbox.selectedButton, "no one-off should be selected anymore");
});

add_task(function* test_tab_and_arrows() {
  
  ok(!textbox.selectedButton, "no one-off button should be selected");
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  is(textbox.value, "", "the textfield value should be unmodified");

  
  let oneOffs = getOneOffs();
  EventUtils.synthesizeKey("VK_DOWN", {});
  is(textbox.selectedButton, oneOffs[0],
     "the first one-off button should be selected");
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");

  
  EventUtils.synthesizeKey("VK_TAB", {});
  is(textbox.selectedButton, oneOffs[1],
     "the second one-off button should be selected");
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");

  
  EventUtils.synthesizeKey("VK_UP", {});
  is(textbox.selectedButton, oneOffs[0],
     "the first one-off button should be selected");
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");

  
  let promise = promiseEvent(searchPopup, "popuphidden");
  searchPopup.hidePopup();
  yield promise;
});

add_task(function* test_open_search() {
  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;

  let deferred = Promise.defer();
  let browser = gBrowser.selectedBrowser;
  browser.addEventListener("load", function onload() {
    browser.removeEventListener("load", onload, true);
    deferred.resolve();
  }, true);

  let rootDir = getRootDirectory(gTestPath);
  content.location = rootDir + "opensearch.html";

  yield deferred.promise;

  let promise = promiseEvent(searchPopup, "popupshown");
  info("Opening search panel");
  EventUtils.synthesizeMouseAtCenter(searchIcon, {});
  yield promise;
  is(searchPopup.getAttribute("showonlysettings"), "true", "Should show the small popup");

  let engines = getOpenSearchItems();
  is(engines.length, 2, "the opensearch.html page exposes 2 engines")

  
  is(searchPopup.selectedIndex, -1, "no suggestion should be selected");
  ok(!textbox.selectedButton, "no button should be selected");

  
  EventUtils.synthesizeKey("VK_UP", {});
  is(textbox.selectedButton.getAttribute("anonid"), "search-settings",
     "the settings item should be selected");

  
  for (let i = engines.length; i; --i) {
    EventUtils.synthesizeKey("VK_UP", {});
    let selectedButton = textbox.selectedButton;
    is(selectedButton, engines[i - 1],
       "the engine #" + i + " should be selected");
    ok(selectedButton.classList.contains("addengine-item"),
       "the button is themed as an engine item");
  }

  
  EventUtils.synthesizeKey("VK_UP", {});
  is(textbox.selectedButton, getOneOffs().pop(),
     "the last one-off button should be selected");

  info("now check that the down key navigates open search items as expected");
  for (let i = 0; i < engines.length; ++i) {
    EventUtils.synthesizeKey("VK_DOWN", {});
    is(textbox.selectedButton, engines[i],
       "the engine #" + (i + 1) + " should be selected");
  }

  
  EventUtils.synthesizeKey("VK_DOWN", {});
  is(textbox.selectedButton.getAttribute("anonid"), "search-settings",
     "the settings item should be selected");

  promise = promiseEvent(searchPopup, "popuphidden");
  searchPopup.hidePopup();
  yield promise;

  gBrowser.removeCurrentTab();
});




const TESTCASE_URI = TEST_BASE + "four.html";

function test() {
  waitForExplicitFinish();

  addTabAndLaunchStyleEditorChromeWhenLoaded(function (aChrome) {
    run(aChrome);
  });

  content.location = TESTCASE_URI;
}

let gSEChrome, timeoutID;

function run(aChrome) {
  gSEChrome = aChrome;
  gBrowser.tabContainer.addEventListener("TabOpen", onTabAdded, false);
  aChrome.editors[0].addActionListener({onAttach: onEditor0Attach});
  aChrome.editors[1].addActionListener({onAttach: onEditor1Attach});
}

function getStylesheetNameLinkFor(aEditor) {
  return gSEChrome.getSummaryElementForEditor(aEditor).querySelector(".stylesheet-name");
}

function onEditor0Attach(aEditor) {
  waitForFocus(function () {
    
    EventUtils.synthesizeMouseAtCenter(
      getStylesheetNameLinkFor(gSEChrome.editors[1]),
      {button: 0},
      gChromeWindow);
  }, gChromeWindow);
}

function onEditor1Attach(aEditor) {
  ok(aEditor.sourceEditor.hasFocus(),
     "left mouse click has given editor 1 focus");

  
  EventUtils.synthesizeMouseAtCenter(
    getStylesheetNameLinkFor(gSEChrome.editors[2]),
    {button: 1},
    gChromeWindow);

  setTimeout(finish, 0);
}

function onTabAdded() {
  ok(false, "middle mouse click has opened a new tab");
  finish();
}

registerCleanupFunction(function () {
  gBrowser.tabContainer.removeEventListener("TabOpen", onTabAdded, false);
  gSEChrome = null;
});

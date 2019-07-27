








thisTestLeaksUncaughtRejectionsAndShouldBeFixed("Error: Unknown sheet source");

let gUI;

const FIRST_TEST_PAGE = TEST_BASE_HTTP + "inline-1.html"
const SECOND_TEST_PAGE = TEST_BASE_HTTP + "inline-2.html"
const SAVE_PATH = "test.css";

function test()
{
  waitForExplicitFinish();

  addTabAndOpenStyleEditors(2, function(panel) {
    gUI = panel.UI;

    
    
    testIndentifierGeneration();

    saveFirstInlineStyleSheet()
    .then(testFriendlyNamesAfterSave)
    .then(reloadPage)
    .then(testFriendlyNamesAfterSave)
    .then(navigateToAnotherPage)
    .then(testFriendlyNamesAfterNavigation)
    .then(finishTests);
  });

  content.location = FIRST_TEST_PAGE;
}

function testIndentifierGeneration() {
  let fakeStyleSheetFile = {
    "href": "http://example.com/test.css",
    "nodeHref": "http://example.com/",
    "styleSheetIndex": 1
  }

  let fakeInlineStyleSheet = {
    "href": null,
    "nodeHref": "http://example.com/",
    "styleSheetIndex": 2
  }

  is(gUI.getStyleSheetIdentifier(fakeStyleSheetFile), "http://example.com/test.css",
    "URI is the identifier of style sheet file.");

  is(gUI.getStyleSheetIdentifier(fakeInlineStyleSheet), "inline-2-at-http://example.com/",
    "Inline style sheets are identified by their page and position at that page.");
}

function saveFirstInlineStyleSheet() {
  let deferred = promise.defer();
  let editor = gUI.editors[0];

  let destFile = FileUtils.getFile("ProfD", [SAVE_PATH]);

  editor.saveToFile(destFile, function (file) {
    ok(file, "File was correctly saved.");
    deferred.resolve();
  });

  return deferred.promise;
}

function testFriendlyNamesAfterSave() {
  let firstEditor = gUI.editors[0];
  let secondEditor = gUI.editors[1];

  
  
  is(firstEditor.friendlyName, SAVE_PATH,
    "Friendly name is correct for the saved inline style sheet.");
  isnot(secondEditor.friendlyName, SAVE_PATH,
    "Friendly name is for the second inline style sheet is not the same as first.");

  return promise.resolve(null);
}

function reloadPage() {
  info("Reloading page.");
  content.location.reload();
  return waitForEditors(2);
}

function navigateToAnotherPage() {
  info("Navigating to another page.");
  let deferred = promise.defer();
  gBrowser.removeCurrentTab();

  gUI = null;

  addTabAndOpenStyleEditors(2, function(panel) {
    gUI = panel.UI;
    deferred.resolve();
  });

  content.location = SECOND_TEST_PAGE;
  return deferred.promise;
}

function testFriendlyNamesAfterNavigation() {
  let firstEditor = gUI.editors[0];
  let secondEditor = gUI.editors[1];

  
  
  isnot(firstEditor.friendlyName, SAVE_PATH,
    "The first editor doesn't have the save path as a friendly name.");
  isnot(secondEditor.friendlyName, SAVE_PATH,
    "The second editor doesn't have the save path as a friendly name.");

  return promise.resolve(null);
}

function finishTests() {
  gUI = null;
  finish();
}









function waitForEditors(aNumberOfEditors) {
  let deferred = promise.defer();
  let count = 0;

  info("Waiting for " + aNumberOfEditors + " editors to be added");
  gUI.on("editor-added", function editorAdded(event, editor) {
    if (++count == aNumberOfEditors) {
        info("All editors added. Resolving promise.");
        gUI.off("editor-added", editorAdded);
        gUI.editors[0].getSourceEditor().then(deferred.resolve);
    }
    else {
        info ("Editor " + count + " of " + aNumberOfEditors + " added.");
    }
  });

  return deferred.promise;
}

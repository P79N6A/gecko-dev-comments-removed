



const {Ci: interfaces, Cc: classes} = Components;

let Clipboard = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
let HasFindClipboard = Clipboard.supportsFindClipboard();

function test() {
  waitForExplicitFinish();

  whenNewWindowLoaded(undefined, function (win) {
    whenDelayedStartupFinished(win, function () {
      let selectedBrowser = win.gBrowser.selectedBrowser;
      selectedBrowser.addEventListener("pageshow", function pageshowListener() {
        selectedBrowser.removeEventListener("pageshow", pageshowListener, true);
        ok(true, "pageshow listener called: " + win.content.location);
        waitForFocus(function () {
          onFocus(win);
        }, selectedBrowser.contentWindow);
      }, true);
      selectedBrowser.loadURI("data:text/html,<h1 id='h1'>Select Me</h1>");
    });
  });
}

function selectText(win) {
  let elt = win.document.getElementById("h1");
  let selection = win.getSelection();
  let range = win.document.createRange();
  range.setStart(elt, 0);
  range.setEnd(elt, 1);
  selection.removeAllRanges();
  selection.addRange(range);
}

function onFocus(win) {
  ok(!win.gFindBarInitialized, "find bar is not yet initialized");
  let findBar = win.gFindBar;
  selectText(win.content);

  findBar.onFindCommand().then(onInitialized.bind(null, findBar, win));
}

function onInitialized(findBar, win) {
  
  
  if (!HasFindClipboard)
    is(findBar._findField.value, "Select Me", "Findbar is initialized with selection");
  findBar.close();
  win.close();
  finish();
}

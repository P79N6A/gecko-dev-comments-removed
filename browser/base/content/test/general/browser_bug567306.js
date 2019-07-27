



const {Ci: interfaces, Cc: classes} = Components;

let Clipboard = Cc["@mozilla.org/widget/clipboard;1"].getService(Ci.nsIClipboard);
let HasFindClipboard = Clipboard.supportsFindClipboard();

add_task(function* () {
  let newwindow = yield BrowserTestUtils.openNewBrowserWindow();

  let selectedBrowser = newwindow.gBrowser.selectedBrowser;
  yield new Promise((resolve, reject) => {
    selectedBrowser.addEventListener("pageshow", function pageshowListener() {
      if (selectedBrowser.currentURI.spec == "about:blank")
        return;

      selectedBrowser.removeEventListener("pageshow", pageshowListener, true);
      ok(true, "pageshow listener called: " + newwindow.content.location);
      resolve();
    }, true);
    selectedBrowser.loadURI("data:text/html,<h1 id='h1'>Select Me</h1>");
  });

  yield SimpleTest.promiseFocus(newwindow);

  ok(!newwindow.gFindBarInitialized, "find bar is not yet initialized");
  let findBar = newwindow.gFindBar;

  yield ContentTask.spawn(selectedBrowser, { }, function* () {
    let elt = content.document.getElementById("h1");
    let selection = content.getSelection();
    let range = content.document.createRange();
    range.setStart(elt, 0);
    range.setEnd(elt, 1);
    selection.removeAllRanges();
    selection.addRange(range);
  });

  yield findBar.onFindCommand();

  
  
  if (!HasFindClipboard)
    is(findBar._findField.value, "Select Me", "Findbar is initialized with selection");
  findBar.close();
  yield promiseWindowClosed(newwindow);
});


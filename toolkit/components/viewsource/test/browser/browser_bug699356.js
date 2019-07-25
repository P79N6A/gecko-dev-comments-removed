



function test() {
  let source = "about:blank";

  waitForExplicitFinish();
  openViewSourceWindow(source, function(aWindow) {
    let gBrowser = aWindow.gBrowser;

    todo(gBrowser.contentDocument.title == source, "Correct document title");
    todo(aWindow.document.documentElement.getAttribute("title") == "Source of: " + source, "Correct window title");
    closeViewSourceWindow(aWindow, finish);
  });
}

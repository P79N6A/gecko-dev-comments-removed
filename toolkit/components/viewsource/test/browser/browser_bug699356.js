



function test() {
  let source = "about:blank";

  waitForExplicitFinish();
  openViewSourceWindow(source, function(aWindow) {
    let gBrowser = aWindow.gBrowser;
    let docEl = aWindow.document.documentElement;

    is(gBrowser.contentDocument.title, source, "Correct document title");
    is(docEl.getAttribute("title"),
      "Source of: " + source + ("nsILocalFileMac" in Components.interfaces ? "" : " - " + docEl.getAttribute("titlemodifier")),
      "Correct window title");
    closeViewSourceWindow(aWindow, finish);
  });
}

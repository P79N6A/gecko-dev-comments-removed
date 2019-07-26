



const Ci = Components.interfaces;

let tab, browser;

function test () {
  waitForExplicitFinish();

  tab = gBrowser.addTab("data:text/html,<iframe srcdoc='content'/>");
  browser = gBrowser.getBrowserForTab(tab);
  gBrowser.selectedTab = tab;

  browser.addEventListener("load", startTests, true);
}

function startTests () {
  browser.removeEventListener("load", startTests, true);

  let finder = browser.finder;
  let listener = {
    onFindResult: function () {
      ok(false, "callback wasn't replaced");
    }
  };
  finder.addResultListener(listener);

  listener.onFindResult = function (result) {
    ok(result == Ci.nsITypeAheadFind.FIND_FOUND, "should find string");

    listener.onFindResult = function (result) {
      ok(result == Ci.nsITypeAheadFind.FIND_NOTFOUND, "should not find string");

      cleanup();
    }
    finder.highlight(true, "Bla");
  }
  finder.highlight(true, "content");
}

function cleanup() {
  gBrowser.removeTab(tab);
  finish();
}

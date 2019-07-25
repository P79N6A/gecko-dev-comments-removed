









































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded",
                           testLiveFilteringOnSearchStrings, false);
}

function testLiveFilteringOnSearchStrings() {
  browser.removeEventListener("DOMContentLoaded",
                              testLiveFilteringOnSearchStrings, false);

  openConsole();

  hud = HUDService.getHudByWindow(content);
  let console = browser.contentWindow.wrappedJSObject.console;

  for (let i = 0; i < 50; i++) {
    console.log("http://www.example.com/");
  }

  setStringFilter("http");
  isnot(countMessageNodes(), 0, "the log nodes are not hidden when the " +
    "search string is set to \"http\"");

  setStringFilter("hxxp");
  is(countMessageNodes(), 0, "the log nodes are hidden when the search " +
    "string is set to \"hxxp\"");

  setStringFilter("ht tp");
  isnot(countMessageNodes(), 0, "the log nodes are not hidden when the " +
    "search string is set to \"ht tp\"");

  setStringFilter(" zzzz   zzzz ");
  is(countMessageNodes(), 0, "the log nodes are hidden when the search " +
    "string is set to \" zzzz   zzzz \"");

  setStringFilter("");
  isnot(countMessageNodes(), 0, "the log nodes are not hidden when the " +
    "search string is removed");

  setStringFilter("\u9f2c");
  is(countMessageNodes(), 0, "the log nodes are hidden when searching " +
    "for weasels");

  setStringFilter("\u0007");
  is(countMessageNodes(), 0, "the log nodes are hidden when searching for " +
    "the bell character");

  setStringFilter('"foo"');
  is(countMessageNodes(), 0, "the log nodes are hidden when searching for " +
    'the string "foo"');

  setStringFilter("'foo'");
  is(countMessageNodes(), 0, "the log nodes are hidden when searching for " +
    "the string 'foo'");

  setStringFilter("foo\"bar'baz\"boo'");
  is(countMessageNodes(), 0, "the log nodes are hidden when searching for " +
    "the string \"foo\"bar'baz\"boo'\"");

  finishTest();
}

function countMessageNodes() {
  let outputNode = hud.outputNode;

  let messageNodes = outputNode.querySelectorAll(".hud-log");
  let displayedMessageNodes = 0;
  let view = hud.chromeWindow;
  for (let i = 0; i < messageNodes.length; i++) {
    let computedStyle = view.getComputedStyle(messageNodes[i], null);
    if (computedStyle.display !== "none") {
      displayedMessageNodes++;
    }
  }

  return displayedMessageNodes;
}

function setStringFilter(aValue)
{
  hud.filterBox.value = aValue;
  HUDService.adjustVisibilityOnSearchStringChange(hud.hudId, aValue);
}





function test()
{
  waitForExplicitFinish();

  let inspector, searchBox, state, panel;

  
  
  
  
  
  let keyStates = [
    ["d", "d"],
    ["i", "di"],
    ["v", "div"],
    [".", "div."],
    ["VK_UP", "div.c1"],
    ["VK_DOWN", "div.l1"],
    ["VK_DOWN", "div.l1"],
    ["VK_BACK_SPACE", "div.l"],
    ["VK_UP", "div.l1"],
    ["VK_UP", "div.l1"],
    [" ", "div.l1 "],
    ["VK_UP", "div.l1 DIV"],
    ["VK_UP", "div.l1 DIV"],
    [".", "div.l1 DIV."],
    ["VK_TAB", "div.l1 DIV.c1"],
    ["VK_BACK_SPACE", "div.l1 DIV.c"],
    ["VK_BACK_SPACE", "div.l1 DIV."],
    ["VK_BACK_SPACE", "div.l1 DIV"],
    ["VK_BACK_SPACE", "div.l1 DI"],
    ["VK_BACK_SPACE", "div.l1 D"],
    ["VK_BACK_SPACE", "div.l1 "],
    ["VK_UP", "div.l1 DIV"],
    ["VK_BACK_SPACE", "div.l1 DI"],
    ["VK_BACK_SPACE", "div.l1 D"],
    ["VK_BACK_SPACE", "div.l1 "],
    ["VK_UP", "div.l1 DIV"],
    ["VK_UP", "div.l1 DIV"],
    ["VK_TAB", "div.l1 DIV"],
    ["VK_BACK_SPACE", "div.l1 DI"],
    ["VK_BACK_SPACE", "div.l1 D"],
    ["VK_BACK_SPACE", "div.l1 "],
    ["VK_DOWN", "div.l1 DIV"],
    ["VK_DOWN", "div.l1 SPAN"],
    ["VK_DOWN", "div.l1 SPAN"],
    ["VK_BACK_SPACE", "div.l1 SPA"],
    ["VK_BACK_SPACE", "div.l1 SP"],
    ["VK_BACK_SPACE", "div.l1 S"],
    ["VK_BACK_SPACE", "div.l1 "],
    ["VK_BACK_SPACE", "div.l1"],
    ["VK_BACK_SPACE", "div.l"],
    ["VK_BACK_SPACE", "div."],
    ["VK_BACK_SPACE", "div"],
    ["VK_BACK_SPACE", "di"],
    ["VK_BACK_SPACE", "d"],
    ["VK_BACK_SPACE", ""],
  ];

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    waitForFocus(setupTest, content);
  }, true);

  content.location = "http://mochi.test:8888/browser/browser/devtools/inspector/test/browser_inspector_bug_831693_search_suggestions.html";

  function $(id) {
    if (id == null) return null;
    return content.document.getElementById(id);
  }

  function setupTest()
  {
    openInspector(startTest);
  }

  function startTest(aInspector)
  {
    inspector = aInspector;
    searchBox =
      inspector.panelWin.document.getElementById("inspector-searchbox");
    panel = inspector.searchSuggestions.searchPopup._list;

    focusSearchBoxUsingShortcut(inspector.panelWin, function() {
      searchBox.addEventListener("keypress", checkState, true);
      panel.addEventListener("keypress", checkState, true);
      checkStateAndMoveOn(0);
    });
  }

  function checkStateAndMoveOn(index) {
    if (index == keyStates.length) {
      finishUp();
      return;
    }

    let [key, query] = keyStates[index];
    state = index;

    info("pressing key " + key + " to get searchbox value as " + query);
    EventUtils.synthesizeKey(key, {}, inspector.panelWin);
  }

  function checkState(event) {
    
    window.setTimeout(function() {
      let [key, query] = keyStates[state];

      is(searchBox.value, query, "The suggestion at " + state + "th step on " +
         "pressing " + key + " key is correct.")
      checkStateAndMoveOn(state + 1);
    }, 200);
  }

  function finishUp() {
    searchBox = null;
    panel = null;
    gBrowser.removeCurrentTab();
    finish();
  }
}

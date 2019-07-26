






function test()
{
  waitForExplicitFinish();

  let inspector, searchBox, state, popup;

  
  
  
  
  
  
  
  let keyStates = [
    ["s", [["span", 1], [".span", 1], ["#span", 1]]],
    ["p", [["span", 1], [".span", 1], ["#span", 1]]],
    ["a", [["span", 1], [".span", 1], ["#span", 1]]],
    ["n", []],
    [" ", [["span div", 1]]],
    ["d", [["span div", 1]]], 
    ["VK_BACK_SPACE", [["span div", 1]]],
    ["VK_BACK_SPACE", []],
    ["VK_BACK_SPACE", [["span", 1], [".span", 1], ["#span", 1]]],
    ["VK_BACK_SPACE", [["span", 1], [".span", 1], ["#span", 1]]],
    ["VK_BACK_SPACE", [["span", 1], [".span", 1], ["#span", 1]]],
    ["VK_BACK_SPACE", []],
    
    
    ["b", [
      ["button", 3],
      ["body", 1],
      [".bc", 3],
      [".ba", 1],
      [".bb", 1],
      ["#ba", 1],
      ["#bb", 1],
      ["#bc", 1]
    ]],
  ];

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    waitForFocus(setupTest, content);
  }, true);

  content.location = "data:text/html," +
                     "<span class='span' id='span'>" +
                     "  <div class='div' id='div'></div>" +
                     "</span>" +
                     "<button class='ba bc' id='bc'></button>" +
                     "<button class='bb bc' id='bb'></button>" +
                     "<button class='bc' id='ba'></button>";

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
    popup = inspector.searchSuggestions.searchPopup;

    focusSearchBoxUsingShortcut(inspector.panelWin, function() {
      searchBox.addEventListener("command", checkState, true);
      checkStateAndMoveOn(0);
    });
  }

  function checkStateAndMoveOn(index) {
    if (index == keyStates.length) {
      finishUp();
      return;
    }

    let [key, suggestions] = keyStates[index];
    state = index;

    info("pressing key " + key + " to get suggestions " +
         JSON.stringify(suggestions));
    EventUtils.synthesizeKey(key, {}, inspector.panelWin);
  }

  function checkState(event) {
    inspector.searchSuggestions._lastQuery.then(() => {
      let [key, suggestions] = keyStates[state];
      let actualSuggestions = popup.getItems();
      is(popup.isOpen ? actualSuggestions.length: 0, suggestions.length,
         "There are expected number of suggestions at " + state + "th step.");
      actualSuggestions.reverse();
      for (let i = 0; i < suggestions.length; i++) {
        is(suggestions[i][0], actualSuggestions[i].label,
           "The suggestion at " + i + "th index for " + state +
           "th step is correct.")
        is(suggestions[i][1] || 1, actualSuggestions[i].count,
           "The count for suggestion at " + i + "th index for " + state +
           "th step is correct.")
      }
      checkStateAndMoveOn(state + 1);
    });
  }

  function finishUp() {
    searchBox = null;
    popup = null;
    gBrowser.removeCurrentTab();
    finish();
  }
}

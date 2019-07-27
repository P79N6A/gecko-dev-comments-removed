


"use strict";





const KEY_STATES = [
  ["d", "d"],
  ["i", "di"],
  ["v", "div"],
  [".", "div."],
  ["VK_UP", "div.c1"],
  ["VK_DOWN", "div.l1"],
  ["VK_DOWN", "div.l1"],
  ["VK_BACK_SPACE", "div.l"],
  ["VK_TAB", "div.l1"],
  [" ", "div.l1 "],
  ["VK_UP", "div.l1 div"],
  ["VK_UP", "div.l1 div"],
  [".", "div.l1 div."],
  ["VK_TAB", "div.l1 div.c1"],
  ["VK_BACK_SPACE", "div.l1 div.c"],
  ["VK_BACK_SPACE", "div.l1 div."],
  ["VK_BACK_SPACE", "div.l1 div"],
  ["VK_BACK_SPACE", "div.l1 di"],
  ["VK_BACK_SPACE", "div.l1 d"],
  ["VK_BACK_SPACE", "div.l1 "],
  ["VK_UP", "div.l1 div"],
  ["VK_BACK_SPACE", "div.l1 di"],
  ["VK_BACK_SPACE", "div.l1 d"],
  ["VK_BACK_SPACE", "div.l1 "],
  ["VK_UP", "div.l1 div"],
  ["VK_UP", "div.l1 div"],
  ["VK_TAB", "div.l1 div"],
  ["VK_BACK_SPACE", "div.l1 di"],
  ["VK_BACK_SPACE", "div.l1 d"],
  ["VK_BACK_SPACE", "div.l1 "],
  ["VK_DOWN", "div.l1 div"],
  ["VK_DOWN", "div.l1 span"],
  ["VK_DOWN", "div.l1 span"],
  ["VK_BACK_SPACE", "div.l1 spa"],
  ["VK_BACK_SPACE", "div.l1 sp"],
  ["VK_BACK_SPACE", "div.l1 s"],
  ["VK_BACK_SPACE", "div.l1 "],
  ["VK_BACK_SPACE", "div.l1"],
  ["VK_BACK_SPACE", "div.l"],
  ["VK_BACK_SPACE", "div."],
  ["VK_BACK_SPACE", "div"],
  ["VK_BACK_SPACE", "di"],
  ["VK_BACK_SPACE", "d"],
  ["VK_BACK_SPACE", ""],
];

const TEST_URL = TEST_URL_ROOT +
  "doc_inspector_search-suggestions.html";

add_task(function* () {
  let { inspector } = yield openInspectorForURL(TEST_URL);
  yield focusSearchBoxUsingShortcut(inspector.panelWin);

  for (let [key, query] of KEY_STATES) {
    info("Pressing key " + key + " to get searchbox value as " + query);

    let done = inspector.searchSuggestions.once("processing-done");
    EventUtils.synthesizeKey(key, {}, inspector.panelWin);
    yield done;

    is(inspector.searchBox.value, query, "The searchbox value is correct.");
  }
});

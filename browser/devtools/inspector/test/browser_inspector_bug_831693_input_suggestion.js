


"use strict";




const TEST_URL = TEST_URL_ROOT + "browser_inspector_bug_650804_search.html";





let TEST_DATA = [
  {
    key: "d",
    suggestions: [{label: "div", count: 2}]
  },
  {
    key: "i",
    suggestions: [{label: "div", count: 2}]
  },
  {
    key: "v",
    suggestions: []
  },
  {
    key: ".",
    suggestions: [{label: "div.c1"}]
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: []
  },
  {
    key: "#",
    suggestions: [
      {label: "div#d1"},
      {label: "div#d2"}
    ]
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: []
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: [{label: "div", count: 2}]
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: [{label: "div", count: 2}]
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: []
  },
  {
    key: ".",
    suggestions: [
      {label: ".c1", count: 3},
      {label: ".c2"}
    ]
  },
  {
    key: "c",
    suggestions: [
      {label: ".c1", count: 3},
      {label: ".c2"}
    ]
  },
  {
    key: "2",
    suggestions: []
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: [
      {label: ".c1", count: 3},
      {label: ".c2"}
    ]
  },
  {
    key: "1",
    suggestions: []
  },
  {
    key: "#",
    suggestions: [
      {label: "#d2"},
      {label: "#p1"},
      {label: "#s2"}
    ]
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: []
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: [
      {label: ".c1", count: 3},
      {label: ".c2"}
    ]
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: [
      {label: ".c1", count: 3},
      {label: ".c2"}
    ]
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: []
  },
  {
    key: "#",
    suggestions: [
      {label: "#b1"},
      {label: "#d1"},
      {label: "#d2"},
      {label: "#p1"},
      {label: "#p2"},
      {label: "#p3"},
      {label: "#s1"},
      {label: "#s2"}
    ]
  },
  {
    key: "p",
    suggestions: [
      {label: "#p1"},
      {label: "#p2"},
      {label: "#p3"}
    ]
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: [
      {label: "#b1"},
      {label: "#d1"},
      {label: "#d2"},
      {label: "#p1"},
      {label: "#p2"},
      {label: "#p3"},
      {label: "#s1"},
      {label: "#s2"}
    ]
  },
  {
    key: "VK_BACK_SPACE",
    suggestions: []
  }
];

let test = asyncTest(function* () {
  let { inspector } = yield openInspectorForURL(TEST_URL);
  let searchBox = inspector.searchBox;
  let popup = inspector.searchSuggestions.searchPopup;

  yield focusSearchBoxUsingShortcut(inspector.panelWin);

  for (let { key, suggestions } of TEST_DATA) {
    info("Pressing " + key + " to get " + formatSuggestions(suggestions));

    let command = once(searchBox, "command");
    EventUtils.synthesizeKey(key, {}, inspector.panelWin);
    yield command;

    info("Waiting for search query to complete");
    yield inspector.searchSuggestions._lastQuery;

    info("Query completed. Performing checks for input '" + searchBox.value + "'");
    let actualSuggestions = popup.getItems().reverse();

    is(popup.isOpen ? actualSuggestions.length: 0, suggestions.length,
       "There are expected number of suggestions.");

    for (let i = 0; i < suggestions.length; i++) {
      is(suggestions[i].label, actualSuggestions[i].label,
         "The suggestion at " + i + "th index is correct.");
      is(suggestions[i].count || 1, actualSuggestions[i].count,
         "The count for suggestion at " + i + "th index is correct.");
    }
  }
});

function formatSuggestions(suggestions) {
  return "[" + suggestions
                .map(s => "'" + s.label + "' (" + s.count || 1 + ")")
                .join(", ") + "]";
}




"use strict";



const TESTCASE_URI = TEST_BASE_HTTP + "autocomplete.html";
const MAX_SUGGESTIONS = 15;

const {CSSProperties, CSSValues} = getCSSKeywords();












let TEST_CASES = [
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['Ctrl+Space', {total: 1, current: 0}],
  ['VK_LEFT'],
  ['VK_RIGHT'],
  ['VK_DOWN'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['Ctrl+Space', { total: getSuggestionNumberFor("font"), current: 0}],
  ['VK_END'],
  ['VK_RETURN'],
  ['b', {total: getSuggestionNumberFor("b"), current: 0}],
  ['a', {total: getSuggestionNumberFor("ba"), current: 0}],
  ['VK_DOWN', {total: getSuggestionNumberFor("ba"), current: 0, inserted: 1}],
  ['VK_TAB', {total: getSuggestionNumberFor("ba"), current: 1, inserted: 1}],
  ['VK_RETURN', {current: 1, inserted: 1, entered: 1}],
  ['b', {total: getSuggestionNumberFor("background", "b"), current: 0}],
  ['l', {total: getSuggestionNumberFor("background", "bl"), current: 0}],
  ['VK_TAB', {total: getSuggestionNumberFor("background", "bl"), current: 0, inserted: 1}],
  ['VK_DOWN', {total: getSuggestionNumberFor("background", "bl"), current: 1, inserted: 1}],
  ['VK_UP', {total: getSuggestionNumberFor("background", "bl"), current: 0, inserted: 1}],
  ['VK_TAB', {total: getSuggestionNumberFor("background", "bl"), current: 1, inserted: 1}],
  ['VK_TAB', {total: getSuggestionNumberFor("background", "bl"), current: 2, inserted: 1}],
  [';'],
  ['VK_RETURN'],
  ['c', {total: getSuggestionNumberFor("c"), current: 0}],
  ['o', {total: getSuggestionNumberFor("co"), current: 0}],
  ['VK_RETURN', {current: 0, inserted: 1}],
  ['r', {total: getSuggestionNumberFor("color", "r"), current: 0}],
  ['VK_RETURN', {current: 0, inserted: 1}],
  [';'],
  ['VK_LEFT'],
  ['VK_RIGHT'],
  ['VK_DOWN'],
  ['VK_RETURN'],
  ['b', {total: 2, current: 0}],
  ['u', {total: 1, current: 0}],
  ['VK_RETURN', {current: 0, inserted: 1}],
  ['{'],
  ['VK_HOME'],
  ['VK_DOWN'],
  ['VK_DOWN'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['VK_RIGHT'],
  ['Ctrl+Space', {total: 1, current: 0}],
];

add_task(function* () {
  let { panel, ui } = yield openStyleEditorForURL(TESTCASE_URI);
  let editor = yield ui.editors[0].getSourceEditor();
  let sourceEditor = editor.sourceEditor;
  let popup = sourceEditor.getAutocompletionPopup();

  yield SimpleTest.promiseFocus(panel.panelWindow);

  for (let index in TEST_CASES) {
    yield testState(index, sourceEditor, popup, panel.panelWindow);
    yield checkState(index, sourceEditor, popup);
  }
});

function testState(index, sourceEditor, popup, panelWindow) {
  let [key, details] = TEST_CASES[index];
  let entered;
  if (details) {
    entered = details.entered;
  }
  let mods = {};

  info("pressing key " + key + " to get result: " +
                JSON.stringify(TEST_CASES[index]) + " for index " + index);

  let evt = "after-suggest";

  if (key == 'Ctrl+Space') {
    key = " ";
    mods.ctrlKey = true;
  }
  else if (key == "VK_RETURN" && entered) {
    evt = "popup-hidden";
  }
  else if (/(left|right|return|home|end)/ig.test(key) ||
           (key == "VK_DOWN" && !popup.isOpen)) {
    evt = "cursorActivity";
  }
  else if (key == "VK_TAB" || key == "VK_UP" || key == "VK_DOWN") {
    evt = "suggestion-entered";
  }

  let ready = sourceEditor.once(evt);
  EventUtils.synthesizeKey(key, mods, panelWindow);

  return ready;
}

function checkState(index, sourceEditor, popup) {
  let deferred = promise.defer();
  executeSoon(() => {
    let [key, details] = TEST_CASES[index];
    details = details || {};
    let {total, current, inserted} = details;

    if (total != undefined) {
      ok(popup.isOpen, "Popup is open for index " + index);
      is(total, popup.itemCount,
         "Correct total suggestions for index " + index);
      is(current, popup.selectedIndex,
         "Correct index is selected for index " + index);
      if (inserted) {
        let { preLabel, label, text } = popup.getItemAtIndex(current);
        let { line, ch } = sourceEditor.getCursor();
        let lineText = sourceEditor.getText(line);
        is(lineText.substring(ch - text.length, ch), text,
           "Current suggestion from the popup is inserted into the editor.");
      }
    }
    else {
      ok(!popup.isOpen, "Popup is closed for index " + index);
      if (inserted) {
        let { preLabel, label, text } = popup.getItemAtIndex(current);
        let { line, ch } = sourceEditor.getCursor();
        let lineText = sourceEditor.getText(line);
        is(lineText.substring(ch - text.length, ch), text,
           "Current suggestion from the popup is inserted into the editor.");
      }
    }
    deferred.resolve();
  });

  return deferred.promise;
}












function getCSSKeywords() {
  let domUtils = Cc["@mozilla.org/inspector/dom-utils;1"]
                   .getService(Ci.inIDOMUtils);
  let props = {};
  let propNames = domUtils.getCSSPropertyNames(domUtils.INCLUDE_ALIASES);
  propNames.forEach(prop => {
    props[prop] = domUtils.getCSSValuesForProperty(prop).sort();
  });
  return {
    CSSValues: props,
    CSSProperties: propNames.sort()
  };
}






function getSuggestionNumberFor(property, value) {
  if (value == null) {
    return CSSProperties.filter(prop => prop.startsWith(property))
                        .slice(0, MAX_SUGGESTIONS).length;
  }
  return CSSValues[property].filter(val => val.startsWith(value))
                            .slice(0, MAX_SUGGESTIONS).length;
}

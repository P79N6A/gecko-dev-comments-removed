








thisTestLeaksUncaughtRejectionsAndShouldBeFixed("Error: Unknown sheet source");

const TESTCASE_URI = TEST_BASE_HTTP + "autocomplete.html";
const MAX_SUGGESTIONS = 15;


const AUTOCOMPLETION_PREF = "devtools.styleeditor.autocompletion-enabled";

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

let gEditor;
let gPopup;
let index = 0;

function test()
{
  waitForExplicitFinish();

  addTabAndOpenStyleEditors(1, testEditorAdded);

  content.location = TESTCASE_URI;
}

function testEditorAdded(panel) {
  info("Editor added, getting the source editor and starting tests");
  panel.UI.editors[0].getSourceEditor().then(editor => {
    info("source editor found, starting tests.");
    gEditor = editor.sourceEditor;
    gPopup = gEditor.getAutocompletionPopup();
    waitForFocus(testState, gPanelWindow);
  });
}

function testState() {
  if (index == TEST_CASES.length) {
    testAutocompletionDisabled();
    return;
  }

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
           (key == "VK_DOWN" && !gPopup.isOpen)) {
    evt = "cursorActivity";
  }
  else if (key == "VK_TAB" || key == "VK_UP" || key == "VK_DOWN") {
    evt = "suggestion-entered";
  }

  gEditor.once(evt, checkState);
  EventUtils.synthesizeKey(key, mods, gPanelWindow);
}

function checkState() {
  executeSoon(() => {
    let [key, details] = TEST_CASES[index];
    details = details || {};
    let {total, current, inserted} = details;

    if (total != undefined) {
      ok(gPopup.isOpen, "Popup is open for index " + index);
      is(total, gPopup.itemCount,
         "Correct total suggestions for index " + index);
      is(current, gPopup.selectedIndex,
         "Correct index is selected for index " + index);
      if (inserted) {
        let { preLabel, label, text } = gPopup.getItemAtIndex(current);
        let { line, ch } = gEditor.getCursor();
        let lineText = gEditor.getText(line);
        is(lineText.substring(ch - text.length, ch), text,
           "Current suggestion from the popup is inserted into the editor.");
      }
    }
    else {
      ok(!gPopup.isOpen, "Popup is closed for index " + index);
      if (inserted) {
        let { preLabel, label, text } = gPopup.getItemAtIndex(current);
        let { line, ch } = gEditor.getCursor();
        let lineText = gEditor.getText(line);
        is(lineText.substring(ch - text.length, ch), text,
           "Current suggestion from the popup is inserted into the editor.");
      }
    }
    index++;
    testState();
  });
}

function testAutocompletionDisabled() {
  gBrowser.removeCurrentTab();

  index = 0;
  info("Starting test to check if autocompletion is disabled correctly.")
  Services.prefs.setBoolPref(AUTOCOMPLETION_PREF, false);

  addTabAndOpenStyleEditors(1, testEditorAddedDisabled);

  content.location = TESTCASE_URI;
}

function testEditorAddedDisabled(panel) {
  info("Editor added, getting the source editor and starting tests");
  panel.UI.editors[0].getSourceEditor().then(editor => {
    is(editor.sourceEditor.getOption("autocomplete"), false,
       "Autocompletion option does not exist");
    ok(!editor.sourceEditor.getAutocompletionPopup(),
       "Autocompletion popup does not exist");
    cleanup();
  });
}

function cleanup() {
  Services.prefs.clearUserPref(AUTOCOMPLETION_PREF);
  gEditor = null;
  gPopup = null;
  finish();
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






"use strict";

const L10N_BUNDLE = "chrome://browser/locale/devtools/sourceeditor.properties";
const L10N = Services.strings.createBundle(L10N_BUNDLE);

const FIND_KEY = L10N.GetStringFromName("find.commandkey");
const FINDAGAIN_KEY = L10N.GetStringFromName("findAgain.commandkey");

const { OS } = Services.appinfo;










const dispatchAndWaitForFocus = (target) => new Promise((resolve) => {
  target.addEventListener("focus", function listener() {
    target.removeEventListener("focus", listener);
    resolve(target);
  });

  target.dispatchEvent(new UIEvent("focus"));
});

function openSearchBox(ed) {
  let edDoc = ed.container.contentDocument;
  let edWin = edDoc.defaultView;

  let input = edDoc.querySelector("input[type=search]");
  ok(!input, "search box closed");

  
  ed.focus();

  EventUtils.synthesizeKey(FINDAGAIN_KEY, { accelKey: true }, edWin);

  input = edDoc.querySelector("input[type=search]");
  ok(input, "find again command key opens the search box");
}

function testFindAgain (ed, inputLine, expectCursor, shiftKey=false) {
  let edDoc = ed.container.contentDocument;
  let edWin = edDoc.defaultView;

  let input = edDoc.querySelector("input[type=search]");
  input.value = inputLine;

  
  
  input.focus();

  EventUtils.synthesizeKey(FINDAGAIN_KEY, { accelKey: true, shiftKey }, edWin);

  ch(ed.getCursor(), expectCursor,
    "find: " + inputLine + " expects cursor: " + expectCursor.toSource());
}

const testSearchBoxTextIsSelected = Task.async(function*(ed) {
  let edDoc = ed.container.contentDocument;
  let edWin = edDoc.defaultView;

  let input = edDoc.querySelector("input[type=search]");
  ok(input, "search box is opened");

  
  
  input.focus();

  
  EventUtils.synthesizeKey("VK_ESCAPE", {}, edWin);

  input = edDoc.querySelector("input[type=search]");
  ok(!input, "search box is closed");

  
  EventUtils.synthesizeKey(FIND_KEY, { accelKey: true }, edWin);

  input = edDoc.querySelector("input[type=search]");
  ok(input, "find command key opens the search box");

  yield dispatchAndWaitForFocus(input);

  let { selectionStart, selectionEnd, value } = input;

  ok(selectionStart === 0 && selectionEnd === value.length,
    "search box's text is selected when re-opened");

  
  input.setSelectionRange(0, 0);

  EventUtils.synthesizeKey(FIND_KEY, { accelKey: true }, edWin);

  ({ selectionStart, selectionEnd } = input);

  ok(selectionStart === 0 && selectionEnd === value.length,
    "search box's text is selected when find key is pressed");

  
  EventUtils.synthesizeKey("VK_ESCAPE", {}, edWin);
});

const testReplaceBoxTextIsSelected = Task.async(function*(ed) {
  let edDoc = ed.container.contentDocument;
  let edWin = edDoc.defaultView;

  let input = edDoc.querySelector(".CodeMirror-dialog > input");
  ok(!input, "dialog box with replace is closed");

  
  ed.focus();

  
  let [altKey, shiftKey] = OS === "Darwin" ? [true, false] : [false, true];

  EventUtils.synthesizeKey(FIND_KEY,
    { accelKey: true, altKey, shiftKey }, edWin);

  input = edDoc.querySelector(".CodeMirror-dialog > input");
  ok(input, "dialog box with replace is opened");

  input.value = "line 5";

  
  
  input.focus();

  yield dispatchAndWaitForFocus(input);

  let { selectionStart, selectionEnd, value } = input;

  ok(!(selectionStart === 0 && selectionEnd === value.length),
    "Text in dialog box is not selected");

  EventUtils.synthesizeKey(FIND_KEY,
    { accelKey: true, altKey, shiftKey }, edWin);

  ({ selectionStart, selectionEnd } = input);

  ok(selectionStart === 0 && selectionEnd === value.length,
    "dialog box's text is selected when replace key is pressed");

  
  EventUtils.synthesizeKey("VK_ESCAPE", {}, edWin);
});

add_task(function*() {
  let { ed, win } = yield setup();

  ed.setText([
    "// line 1",
    "//  line 2",
    "//   line 3",
    "//    line 4",
    "//     line 5"
  ].join("\n"));

  yield promiseWaitForFocus();

  openSearchBox(ed);

  let testVectors = [
    
    
    ["line",
     {line: 0, ch: 7}],
    ["line",
     {line: 1, ch: 8}],
    ["line",
     {line: 2, ch: 9}],
    ["line",
     {line: 3, ch: 10}],
    ["line",
     {line: 4, ch: 11}],
    ["ne 3",
     {line: 2, ch: 11}],
    ["line 1",
      {line: 0, ch: 9}],
    
    ["line",
      {line: 4, ch: 11},
      true],
    ["line",
      {line: 3, ch: 10},
      true],
    ["line",
      {line: 2, ch: 9},
      true],
    ["line",
      {line: 1, ch: 8},
      true],
    ["line",
      {line: 0, ch: 7},
      true]
  ];

  for (let v of testVectors) {
    yield testFindAgain(ed, ...v);
  }

  yield testSearchBoxTextIsSelected(ed);

  yield testReplaceBoxTextIsSelected(ed);

  teardown(ed, win);
});

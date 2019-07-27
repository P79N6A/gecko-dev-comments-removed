




const TESTCASE_URI = TEST_BASE_HTTPS + "media-rules.html";
const MEDIA_PREF = "devtools.styleeditor.showMediaSidebar";

const RESIZE = 300;
const LABELS = ["not all", "all", "(max-width: 400px)", "(max-width: 600px)"];
const LINE_NOS = [1, 7, 19, 25];
const NEW_RULE = "\n@media (max-width: 600px) { div { color: blue; } }";

waitForExplicitFinish();

let test = asyncTest(function*() {
  let {UI} = yield addTabAndOpenStyleEditors(2, null, TESTCASE_URI);

  is(UI.editors.length, 2, "correct number of editors");

  
  let plainEditor = UI.editors[0];
  yield openEditor(plainEditor);
  testPlainEditor(plainEditor);

  
  let mediaEditor = UI.editors[1];
  yield openEditor(mediaEditor);
  testMediaEditor(mediaEditor);

  
  yield testShowHide(UI, mediaEditor);

  
  yield testMediaRuleAdded(UI, mediaEditor);

  
  let originalWidth = window.outerWidth;
  let originalHeight = window.outerHeight;

  let onMatchesChange = listenForMediaChange(UI);
  window.resizeTo(RESIZE, RESIZE);
  yield onMatchesChange;

  testMediaMatchChanged(mediaEditor);

  window.resizeTo(originalWidth, originalHeight);
});

function testPlainEditor(editor) {
  let sidebar = editor.details.querySelector(".stylesheet-sidebar");
  is(sidebar.hidden, true, "sidebar is hidden on editor without @media");
}

function testMediaEditor(editor) {
  let sidebar = editor.details.querySelector(".stylesheet-sidebar");
  is(sidebar.hidden, false, "sidebar is showing on editor with @media");

  let entries = [...sidebar.querySelectorAll(".media-rule-label")];
  is(entries.length, 3, "three @media rules displayed in sidebar");

  testRule(entries[0], LABELS[0], false, LINE_NOS[0]);
  testRule(entries[1], LABELS[1], true, LINE_NOS[1]);
  testRule(entries[2], LABELS[2], false, LINE_NOS[2]);
}

function testMediaMatchChanged(editor) {
  let sidebar = editor.details.querySelector(".stylesheet-sidebar");

  let cond = sidebar.querySelectorAll(".media-rule-condition")[2];
  is(cond.textContent, "(max-width: 400px)", "third rule condition text is correct");
  ok(!cond.classList.contains("media-condition-unmatched"),
     "media rule is now matched after resizing");
}

function* testShowHide(UI, editor) {
  let sidebarChange = listenForMediaChange(UI);
  Services.prefs.setBoolPref(MEDIA_PREF, false);
  yield sidebarChange;

  let sidebar = editor.details.querySelector(".stylesheet-sidebar");
  is(sidebar.hidden, true, "sidebar is hidden after flipping pref");

  sidebarChange = listenForMediaChange(UI);
  Services.prefs.clearUserPref(MEDIA_PREF);
  yield sidebarChange;

  is(sidebar.hidden, false, "sidebar is showing after flipping pref back");
}

function* testMediaRuleAdded(UI, editor) {
  yield editor.getSourceEditor();
  let text = editor.sourceEditor.getText();
  text += NEW_RULE;

  let listChange = listenForMediaChange(UI);
  editor.sourceEditor.setText(text);
  yield listChange;

  let sidebar = editor.details.querySelector(".stylesheet-sidebar");
  let entries = [...sidebar.querySelectorAll(".media-rule-label")];
  is(entries.length, 4, "four @media rules after changing text");

  testRule(entries[3], LABELS[3], false, LINE_NOS[3]);
}

function testRule(rule, text, matches, lineno) {
  let cond = rule.querySelector(".media-rule-condition");
  is(cond.textContent, text, "media label is correct for " + text);

  let matched = !cond.classList.contains("media-condition-unmatched");
  ok(matches ? matched : !matched,
     "media rule is " + (matches ? "matched" : "unmatched"));

  let line = rule.querySelector(".media-rule-line");
  is(line.textContent, ":" + lineno, "correct line number shown");
}



function openEditor(editor) {
  getLinkFor(editor).click();

  return editor.getSourceEditor();
}

function listenForMediaChange(UI) {
  let deferred = promise.defer();
  UI.once("media-list-changed", () => {
    deferred.resolve();
  })
  return deferred.promise;
}

function getLinkFor(editor) {
  return editor.summary.querySelector(".stylesheet-name");
}

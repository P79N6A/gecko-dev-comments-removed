








const TESTCASE_URI = TEST_BASE_HTTPS + "sourcemaps-large.html";

add_task(function*() {
  let {UI} = yield addTabAndOpenStyleEditors(2, null, TESTCASE_URI);

  yield openEditor(UI.editors[0]);
  let iframes = UI.selectedEditor.details.querySelectorAll("iframe");

  is (iframes.length, 1, "There is only one editor iframe");
  ok (UI.selectedEditor.summary.classList.contains("splitview-active"),
    "The editor is selected");
});

function openEditor(editor) {
  getLinkFor(editor).click();

  return editor.getSourceEditor();
}

function getLinkFor(editor) {
  return editor.summary.querySelector(".stylesheet-name");
}
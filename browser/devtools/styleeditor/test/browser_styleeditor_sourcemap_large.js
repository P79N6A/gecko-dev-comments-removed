








const TESTCASE_URI = TEST_BASE_HTTPS + "sourcemaps-large.html";

add_task(function*() {
  let { ui } = yield openStyleEditorForURL(TESTCASE_URI);

  yield openEditor(ui.editors[0]);
  let iframes = ui.selectedEditor.details.querySelectorAll("iframe");

  is (iframes.length, 1, "There is only one editor iframe");
  ok (ui.selectedEditor.summary.classList.contains("splitview-active"),
    "The editor is selected");
});

function openEditor(editor) {
  getLinkFor(editor).click();

  return editor.getSourceEditor();
}

function getLinkFor(editor) {
  return editor.summary.querySelector(".stylesheet-name");
}

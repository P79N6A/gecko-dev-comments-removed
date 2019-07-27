



"use strict";

loadHelperScript("helper_edits.js");


let test = asyncTest(function*() {
  let projecteditor = yield addProjectEditorTabForTempDirectory();
  ok(true, "ProjectEditor has loaded");

  let resources = projecteditor.project.allResources();
  yield selectFile(projecteditor, resources[2]);
  let editor = projecteditor.currentEditor;
  let originalText = editor.editor.getText();

  ok (projecteditor.confirmUnsaved(), "When there are no unsaved changes, confirmUnsaved() is true");
  editor.editor.setText("bar");
  editor.editor.setText(originalText);
  ok (projecteditor.confirmUnsaved(), "When an editor has changed but is still the original text, confirmUnsaved() is true");

  editor.editor.setText("bar");

  checkConfirm(projecteditor);
});

function checkConfirm(projecteditor, container) {
  function confirmYes(aSubject) {
    info("confirm dialog observed as expected, going to click OK");
    Services.obs.removeObserver(confirmYes, "common-dialog-loaded");
    Services.obs.removeObserver(confirmYes, "tabmodal-dialog-loaded");
    aSubject.Dialog.ui.button0.click();
  }

  function confirmNo(aSubject) {
    info("confirm dialog observed as expected, going to click cancel");
    Services.obs.removeObserver(confirmNo, "common-dialog-loaded");
    Services.obs.removeObserver(confirmNo, "tabmodal-dialog-loaded");
    aSubject.Dialog.ui.button1.click();
  }

  Services.obs.addObserver(confirmYes, "common-dialog-loaded", false);
  Services.obs.addObserver(confirmYes, "tabmodal-dialog-loaded", false);

  ok (projecteditor.confirmUnsaved(), "When there are no unsaved changes, clicking OK makes confirmUnsaved() true");

  Services.obs.addObserver(confirmNo, "common-dialog-loaded", false);
  Services.obs.addObserver(confirmNo, "tabmodal-dialog-loaded", false);

  ok (!projecteditor.confirmUnsaved(), "When there are no unsaved changes, clicking cancel makes confirmUnsaved() false");
}




"use strict";




const TESTCASE_URI = TEST_BASE_HTTP + "nostyle.html";

add_task(function* () {
  let { panel } = yield openStyleEditorForURL(TESTCASE_URI);
  let { panelWindow } = panel;

  let root = panelWindow.document.querySelector(".splitview-root");
  ok(!root.classList.contains("loading"),
     "style editor root element does not have 'loading' class name anymore");

  ok(root.querySelector(".empty.placeholder"), "showing 'no style' indicator");

  let button = panelWindow.document.querySelector(".style-editor-newButton");
  ok(!button.hasAttribute("disabled"),
     "new style sheet button is enabled");

  button = panelWindow.document.querySelector(".style-editor-importButton");
  ok(!button.hasAttribute("disabled"),
     "import button is enabled");
});

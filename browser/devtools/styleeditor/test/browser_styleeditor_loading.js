


"use strict";



const TESTCASE_URI = TEST_BASE_HTTP + "longload.html";

add_task(function* () {
  
  
  
  
  let tabAdded = addTab(TESTCASE_URI);
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let styleEditorLoaded = gDevTools.showToolbox(target, "styleeditor");

  yield Promise.all([tabAdded, styleEditorLoaded]);

  let toolbox = gDevTools.getToolbox(target);
  let panel = toolbox.getPanel("styleeditor");
  let { panelWindow } = panel;

  let root = panelWindow.document.querySelector(".splitview-root");
  ok(!root.classList.contains("loading"),
     "style editor root element does not have 'loading' class name anymore");

  let button = panelWindow.document.querySelector(".style-editor-newButton");
  ok(!button.hasAttribute("disabled"),
     "new style sheet button is enabled");

  button = panelWindow.document.querySelector(".style-editor-importButton");
  ok(!button.hasAttribute("disabled"),
     "import button is enabled");
});

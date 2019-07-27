



const TESTCASE_URI = TEST_BASE_HTTP + "longload.html";


function test()
{
  waitForExplicitFinish();

  
  
  
  

  addTabAndCheckOnStyleEditorAdded(function(panel) {
    content.location = TESTCASE_URI;
  }, testEditorAdded);
}

function testEditorAdded(event, editor)
{
  let root = gPanelWindow.document.querySelector(".splitview-root");
  ok(!root.classList.contains("loading"),
     "style editor root element does not have 'loading' class name anymore");

  let button = gPanelWindow.document.querySelector(".style-editor-newButton");
  ok(!button.hasAttribute("disabled"),
     "new style sheet button is enabled");

  button = gPanelWindow.document.querySelector(".style-editor-importButton");
  ok(!button.hasAttribute("disabled"),
     "import button is enabled");

  finish();
}

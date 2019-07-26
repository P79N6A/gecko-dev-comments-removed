



const TESTCASE_URI = TEST_BASE + "nostyle.html";


function test()
{
  waitForExplicitFinish();

  
  
  
  

  addTabAndOpenStyleEditor(function(panel) {
    panel.UI.once("document-load", testDocumentLoad);

    content.location = TESTCASE_URI;
  });
}

function testDocumentLoad(event)
{
  let root = gPanelWindow.document.querySelector(".splitview-root");
  ok(!root.classList.contains("loading"),
     "style editor root element does not have 'loading' class name anymore");

  ok(root.querySelector(".empty.placeholder"), "showing 'no style' indicator");

  let button = gPanelWindow.document.querySelector(".style-editor-newButton");
  ok(!button.hasAttribute("disabled"),
     "new style sheet button is enabled");

  button = gPanelWindow.document.querySelector(".style-editor-importButton");
  ok(!button.hasAttribute("disabled"),
     "import button is enabled");

  finish();
}

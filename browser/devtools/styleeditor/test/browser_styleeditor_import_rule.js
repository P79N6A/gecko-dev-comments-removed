




const TESTCASE_URI = TEST_BASE_HTTP + "import.html";

let gUI;

function test()
{
  waitForExplicitFinish();

  addTabAndOpenStyleEditor(function(panel) {
    gUI = panel.UI;
    gUI.on("editor-added", onEditorAdded);
  });

  content.location = TESTCASE_URI;
}

let gAddedCount = 0;
function onEditorAdded()
{
  if (++gAddedCount != 3) {
    return;
  }

  is(gUI.editors.length, 3,
    "there are 3 stylesheets after loading @imports");

  is(gUI.editors[0].styleSheet.href, TEST_BASE_HTTP + "simple.css",
    "stylesheet 1 is simple.css");

  is(gUI.editors[1].styleSheet.href, TEST_BASE_HTTP + "import.css",
    "stylesheet 2 is import.css");

  is(gUI.editors[2].styleSheet.href, TEST_BASE_HTTP + "import2.css",
    "stylesheet 3 is import2.css");

  gUI = null;
  finish();
}

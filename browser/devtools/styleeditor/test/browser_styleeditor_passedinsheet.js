



const TESTCASE_URI = TEST_BASE + "simple.html";
const LINE = 6;
const COL = 2;

function test()
{
  let editor = null;
  let sheet = null;

  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    run();
  }, true);
  content.location = TESTCASE_URI;

  function run()
  {
    sheet = content.document.styleSheets[1];
    launchStyleEditorChrome(function attachListeners(aChrome) {
      aChrome.addChromeListener({
        onEditorAdded: checkSourceEditor
      });
    }, sheet, LINE, COL);
  }

  function checkSourceEditor(aChrome, aEditor)
  {
    aChrome.removeChromeListener(this);
    if (!aEditor.sourceEditor) {
      aEditor.addActionListener({
        onAttach: function (aEditor) {
          aEditor.removeActionListener(this);
          validate(aEditor);
        }
      });
    } else {
      validate(aEditor);
    }
  }

  function validate(aEditor)
  {
    info("validating style editor");
    let sourceEditor = aEditor.sourceEditor;
    let caretPosition = sourceEditor.getCaretPosition();
    is(caretPosition.line, LINE - 1, "caret row is correct"); 
    is(caretPosition.col, COL - 1, "caret column is correct");
    is(aEditor.styleSheet, sheet, "loaded stylesheet matches document stylesheet");
    finish();
  }
}

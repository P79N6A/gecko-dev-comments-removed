



const TESTCASE_URI = TEST_BASE + "simple.html";

let gOriginalWidth; 
let gOriginalHeight;

function test()
{
  waitForExplicitFinish();

  addTabAndOpenStyleEditor(function(panel) {
    let UI = panel.UI;
    UI.on("editor-added", function(event, editor) {
      if (editor == UI.editors[1]) {
        
        runTests(UI);
      }
    });
  });

  content.location = TESTCASE_URI;
}

function runTests(aUI)
{
  is(aUI.editors.length, 2,
     "there is 2 stylesheets initially");

  aUI.editors[0].getSourceEditor().then(function onEditorAttached(aEditor) {
    executeSoon(function () {
      waitForFocus(function () {
        
        
        let originalSourceEditor = aEditor.sourceEditor;
        aEditor.sourceEditor.setCaretOffset(4); 

        gOriginalWidth = gPanelWindow.outerWidth;
        gOriginalHeight = gPanelWindow.outerHeight;
        gPanelWindow.resizeTo(120, 480);

        executeSoon(function () {
          is(aEditor.sourceEditor, originalSourceEditor,
             "the editor still references the same SourceEditor instance");
          is(aEditor.sourceEditor.getCaretOffset(), 4,
             "the caret position has been preserved");

          
          waitForFocus(function () {
            gPanelWindow.resizeTo(gOriginalWidth, gOriginalHeight);
            executeSoon(function () {
              finish();
            });
          }, gPanelWindow);
        });
      }, gPanelWindow);
    });
  });
}

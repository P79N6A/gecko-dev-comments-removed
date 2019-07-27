



const TESTCASE_URI = TEST_BASE_HTTP + "simple.html";

let gOriginalWidth; 
let gOriginalHeight;

function test()
{
  waitForExplicitFinish();

  addTabAndOpenStyleEditors(2, panel => runTests(panel.UI));

  content.location = TESTCASE_URI;
}

function runTests(aUI)
{
  is(aUI.editors.length, 2,
     "there is 2 stylesheets initially");

  aUI.editors[0].getSourceEditor().then(aEditor => {
    executeSoon(function () {
      waitForFocus(function () {
        
        
        let originalSourceEditor = aEditor.sourceEditor;
        let editor = aEditor.sourceEditor;
        editor.setCursor(editor.getPosition(4)); 

        gOriginalWidth = gPanelWindow.outerWidth;
        gOriginalHeight = gPanelWindow.outerHeight;
        gPanelWindow.resizeTo(120, 480);

        executeSoon(function () {
          is(aEditor.sourceEditor, originalSourceEditor,
             "the editor still references the same Editor instance");
          let editor = aEditor.sourceEditor;
          is(editor.getOffset(editor.getCursor()), 4,
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





const TESTCASE_URI = TEST_BASE + "simple.html";

let gOriginalWidth; 
let gOriginalHeight;

function test()
{
  waitForExplicitFinish();

  addTabAndLaunchStyleEditorChromeWhenLoaded(function (aChrome) {
    if (aChrome.isContentAttached) {
      run(aChrome);
    } else {
      aChrome.addChromeListener({
        onContentAttach: run
      });
    }
  });

  content.location = TESTCASE_URI;
}

function run(aChrome)
{
  is(aChrome.editors.length, 2,
     "there is 2 stylesheets initially");

  aChrome.editors[0].addActionListener({
    onAttach: function onEditorAttached(aEditor) {
      let originalSourceEditor = aEditor.sourceEditor;
      aEditor.sourceEditor.setCaretOffset(4); 

      
      
      executeSoon(function () {
        waitForFocus(function () {
          gOriginalWidth = gChromeWindow.outerWidth;
          gOriginalHeight = gChromeWindow.outerHeight;
          gChromeWindow.resizeTo(120, 480);

          executeSoon(function () {
            is(aEditor.sourceEditor, originalSourceEditor,
               "the editor still references the same SourceEditor instance");
            is(aEditor.sourceEditor.getCaretOffset(), 4,
               "the caret position has been preserved");

            
            waitForFocus(function () {
              gChromeWindow.resizeTo(gOriginalWidth, gOriginalHeight);
              executeSoon(function () {
                finish();
              });
            }, gChromeWindow);
          });
        }, gChromeWindow);
      });
    }
  });
}

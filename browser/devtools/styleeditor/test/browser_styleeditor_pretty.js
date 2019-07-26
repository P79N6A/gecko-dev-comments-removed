



const TESTCASE_URI = TEST_BASE + "minified.html";


function test()
{
  waitForExplicitFinish();

  addTabAndLaunchStyleEditorChromeWhenLoaded(function (aChrome) {
    aChrome.addChromeListener({
      onEditorAdded: function (aChrome, aEditor) {
        if (aEditor.sourceEditor) {
          run(aEditor); 
        } else {
          aEditor.addActionListener({
            onAttach: run
          });
        }
      }
    });
  });

  content.location = TESTCASE_URI;
}

let editorTestedCount = 0;
function run(aEditor)
{
  if (aEditor.styleSheetIndex == 0) {
    let prettifiedSource = "body\{\r?\n\tbackground\:white;\r?\n\}\r?\n\r?\ndiv\{\r?\n\tfont\-size\:4em;\r?\n\tcolor\:red\r?\n\}\r?\n";
    let prettifiedSourceRE = new RegExp(prettifiedSource);

    ok(prettifiedSourceRE.test(aEditor.sourceEditor.getText()),
       "minified source has been prettified automatically");
    editorTestedCount++;
    let chrome = gChromeWindow.styleEditorChrome;
    let summary = chrome.getSummaryElementForEditor(chrome.editors[1]);
    EventUtils.synthesizeMouseAtCenter(summary, {}, gChromeWindow);
  }

  if (aEditor.styleSheetIndex == 1) {
    let originalSource = "body \{ background\: red; \}\r?\ndiv \{\r?\nfont\-size\: 5em;\r?\ncolor\: red\r?\n\}";
    let originalSourceRE = new RegExp(originalSource);

    ok(originalSourceRE.test(aEditor.sourceEditor.getText()),
       "non-minified source has been left untouched");
    editorTestedCount++;
  }

  if (editorTestedCount == 2) {
    finish();
  }
}

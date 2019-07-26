




const TESTCASE_URI = TEST_BASE_HTTPS + "sourcemaps.html";
const PREF = "devtools.styleeditor.source-maps-enabled";

function test()
{
  waitForExplicitFinish();

  Services.prefs.setBoolPref(PREF, true);

  let count = 0;
  addTabAndOpenStyleEditor(function(panel) {
    let UI = panel.UI;
    UI.on("editor-added", (event, editor) => {
      if (++count >= 3) {
        
        
        
        runTests(UI);
      }
    })
  });

  content.location = TESTCASE_URI;
}

function runTests(UI)
{
  is(UI.editors.length, 2);

  let firstEditor = UI.editors[0];
  testFirstEditor(firstEditor);

  let ScssEditor = UI.editors[1];

  let link = getStylesheetNameLinkFor(ScssEditor);
  link.click();

  ScssEditor.getSourceEditor().then(() => {
    testScssEditor(ScssEditor);

    finishUp();
  });
}

function testFirstEditor(editor) {
  let name = getStylesheetNameFor(editor);
  is(name, "simple.css", "First style sheet display name is correct");
}

function testScssEditor(editor) {
  let name = getStylesheetNameFor(editor);
  is(name, "sourcemaps.scss", "Original source display name is correct");

  let text = editor.sourceEditor.getText();

  is(text, "\n\
$paulrougetpink: #f06;\n\
\n\
div {\n\
  color: $paulrougetpink;\n\
}\n\
\n\
span {\n\
  background-color: #EEE;\n\
}", "Original source text is correct");
}


function getStylesheetNameLinkFor(editor) {
  return editor.summary.querySelector(".stylesheet-name");
}

function getStylesheetNameFor(editor) {
  return editor.summary.querySelector(".stylesheet-name > label")
         .getAttribute("value")
}

function finishUp() {
  Services.prefs.clearUserPref(PREF);
  finish();
}
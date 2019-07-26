



const TESTCASE_URI = TEST_BASE_HTTPS + "simple.html";
const NEW_URI = TEST_BASE_HTTPS + "media.html";

const LINE_NO = 5;
const COL_NO  = 0;

let gContentWin;
let gUI;

function test()
{
  waitForExplicitFinish();

  addTabAndOpenStyleEditor(function(panel) {
    gContentWin = gBrowser.selectedTab.linkedBrowser.contentWindow.wrappedJSObject;
    gUI = panel.UI;

    let count = 0;
    gUI.on("editor-added", function editorAdded(event, editor) {
      if (++count == 2) {
        gUI.off("editor-added", editorAdded);
        gUI.editors[0].getSourceEditor().then(runTests);
      }
    })
  });

  content.location = TESTCASE_URI;
}

function runTests()
{
  let count = 0;

  
  
  gUI.once("editor-selected", (event, editor) => {
    editor.getSourceEditor().then(() => {
      is(gUI.selectedEditor, gUI.editors[1], "second editor is selected");
      let {line, ch} = gUI.selectedEditor.sourceEditor.getCursor();
      
      is(line, LINE_NO, "correct line selected");
      is(ch, COL_NO, "correct column selected");

      gUI = null;
      finish();
    });
  });

  gUI.selectStyleSheet(gUI.editors[1].styleSheet.href, LINE_NO);
}
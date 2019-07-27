








thisTestLeaksUncaughtRejectionsAndShouldBeFixed("TypeError: summary is undefined");

const TESTCASE_URI = TEST_BASE_HTTP + "simple.html";

let gUI;

function test()
{
  waitForExplicitFinish();

  addTabAndCheckOnStyleEditorAdded(panel => gUI = panel.UI, testEditorAdded);

  content.location = TESTCASE_URI;
}

let gEditorAddedCount = 0;
function testEditorAdded(aEditor)
{
  if (aEditor.styleSheet.styleSheetIndex == 0) {
    gEditorAddedCount++;
    gUI.editors[0].getSourceEditor().then(testFirstStyleSheetEditor);
  }
  if (aEditor.styleSheet.styleSheetIndex == 1) {
    gEditorAddedCount++;
    testSecondStyleSheetEditor(aEditor);
  }

  if (gEditorAddedCount == 2) {
    gUI = null;
    finish();
  }
}

function testFirstStyleSheetEditor(aEditor)
{
  
  ok(aEditor._state.text.indexOf("\u263a") >= 0,
     "stylesheet is unicode-aware.");

  
  is(aEditor.styleSheet.styleSheetIndex, 0,
     "first stylesheet is at index 0");

  is(aEditor, gUI.editors[0],
     "first stylesheet corresponds to StyleEditorChrome.editors[0]");

  let summary = aEditor.summary;

  let name = summary.querySelector(".stylesheet-name > label").getAttribute("value");
  is(name, "simple.css",
     "first stylesheet's name is `simple.css`");

  let ruleCount = summary.querySelector(".stylesheet-rule-count").textContent;
  is(parseInt(ruleCount), 1,
     "first stylesheet UI shows rule count as 1");

  ok(summary.classList.contains("splitview-active"),
     "first stylesheet UI is focused/active");
}

function testSecondStyleSheetEditor(aEditor)
{
  
  is(aEditor.styleSheet.styleSheetIndex, 1,
     "second stylesheet is at index 1");

  is(aEditor, gUI.editors[1],
     "second stylesheet corresponds to StyleEditorChrome.editors[1]");

  let summary = aEditor.summary;

  let name = summary.querySelector(".stylesheet-name > label").getAttribute("value");
  ok(/^<.*>$/.test(name),
     "second stylesheet's name is surrounded by `<>`");

  let ruleCount = summary.querySelector(".stylesheet-rule-count").textContent;
  is(parseInt(ruleCount), 3,
     "second stylesheet UI shows rule count as 3");

  ok(!summary.classList.contains("splitview-active"),
     "second stylesheet UI is NOT focused/active");
}

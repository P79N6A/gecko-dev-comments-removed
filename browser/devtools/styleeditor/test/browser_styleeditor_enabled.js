




const TESTCASE_URI = TEST_BASE_HTTPS + "simple.html";

function test()
{
  waitForExplicitFinish();

  let count = 0;
  addTabAndOpenStyleEditor(function(panel) {
    let UI = panel.UI;
    UI.on("editor-added", function(event, editor) {
      count++;
      if (count == 2) {
        
        let editor = UI.editors[0];
        editor.getSourceEditor().then(runTests.bind(this, UI, editor));
      }
    })
  });

  content.location = TESTCASE_URI;
}

function runTests(UI, editor)
{
  testEnabledToggle(UI, editor);
}

function testEnabledToggle(UI, editor)
{
  let summary = editor.summary;
  let enabledToggle = summary.querySelector(".stylesheet-enabled");
  ok(enabledToggle, "enabled toggle button exists");

  is(editor.styleSheet.disabled, false,
     "first stylesheet is initially enabled");

  is(summary.classList.contains("disabled"), false,
     "first stylesheet is initially enabled, UI does not have DISABLED class");

  let disabledToggleCount = 0;
  editor.on("property-change", function(event, property) {
    if (property != "disabled") {
      return;
    }
    disabledToggleCount++;

    if (disabledToggleCount == 1) {
      is(editor.styleSheet.disabled, true, "first stylesheet is now disabled");
      is(summary.classList.contains("disabled"), true,
         "first stylesheet is now disabled, UI has DISABLED class");

      
      waitForFocus(function () {
        EventUtils.synthesizeMouseAtCenter(enabledToggle, {}, gPanelWindow);
      }, gPanelWindow);
      return;
    }

    
    is(editor.styleSheet.disabled, false, "first stylesheet is now enabled again");
    is(summary.classList.contains("disabled"), false,
       "first stylesheet is now enabled again, UI does not have DISABLED class");

    finish();
  });

  waitForFocus(function () {
    EventUtils.synthesizeMouseAtCenter(enabledToggle, {}, gPanelWindow);
  }, gPanelWindow);
}

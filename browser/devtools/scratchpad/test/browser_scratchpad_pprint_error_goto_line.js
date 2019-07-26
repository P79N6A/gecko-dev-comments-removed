




"use strict";

function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openScratchpad(runTests);
  }, true);

  content.location = "data:text/html;charset=utf8,test Scratchpad pretty print error goto line.";
}

function testJumpToPrettyPrintError(sp, error, remark) {
  info("will test jumpToLine after prettyPrint error" + remark);
    
    
    is(/Invalid regexp flag \(3:10\)/.test(error), true, "prettyPrint expects error in editor text:\n" + error);
    const errorLine = 3, errorColumn = 10;
    const editorDoc = sp.editor.container.contentDocument;
    sp.editor.jumpToLine();
    const lineInput = editorDoc.querySelector("input");
    const errorLocation = lineInput.value;
    const [ inputLine, inputColumn ] = errorLocation.split(":");
    is(inputLine, errorLine, "jumpToLine input field is set from editor selection (line)");
    is(inputColumn, errorColumn, "jumpToLine input field is set from editor selection (column)");
    EventUtils.synthesizeKey("VK_RETURN", { }, editorDoc.defaultView);
    
    
    const cursor = sp.editor.getCursor();
    is(inputLine, cursor.line + 1, "jumpToLine goto error location (line)");
    is(inputColumn, cursor.ch + 1, "jumpToLine goto error location (column)");
}

function runTests(sw, sp)
{
  sp.setText([
    "// line 1",
    "// line 2",
    "var re = /a bad /regexp/; // line 3 is an obvious syntax error!",
    "// line 4",
    "// line 5",
    ""
  ].join("\n"));
  sp.prettyPrint().then(aFulfill => {
    ok(false, "Expecting Invalid regexp flag (3:10)");
    finish();
  }, error => {
    testJumpToPrettyPrintError(sp, error, " (Bug 1005471, first time)");
  });
  sp.prettyPrint().then(aFulfill => {
    ok(false, "Expecting Invalid regexp flag (3:10)");
    finish();
  }, error => {
    
    testJumpToPrettyPrintError(sp, error, " (second time)");
    finish();
  });
}

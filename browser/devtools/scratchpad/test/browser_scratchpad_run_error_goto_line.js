




"use strict";

function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openScratchpad(runTests);
  }, true);

  content.location = "data:text/html;charset=utf8,test Scratchpad pretty print.";
}

function runTests(sw)
{
  const sp = sw.Scratchpad;
  sp.setText([
    "// line 1",
    "// line 2",
    "var re = /a bad /regexp/; // line 3 is an obvious syntax error!",
    "// line 4",
    "// line 5",
    ""
  ].join("\n"));
  sp.run().then(() => {
    
    
    let errorLine = 3;
    let editorDoc = sp.editor.container.contentDocument;
    sp.editor.jumpToLine();
    let lineInput = editorDoc.querySelector("input");
    let inputLine = lineInput.value;
    is(inputLine, errorLine, "jumpToLine input field is set from editor selection");
    EventUtils.synthesizeKey("VK_RETURN", { }, editorDoc.defaultView);
    
    
    let cursor = sp.editor.getCursor();
    is(cursor.line + 1, inputLine, "jumpToLine goto error location (line)");
    is(cursor.ch + 1, 1, "jumpToLine goto error location (column)");
    finish();
  });
}

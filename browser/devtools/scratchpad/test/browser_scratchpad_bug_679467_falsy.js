




let gScratchpadWindow;

function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    gScratchpadWindow = Scratchpad.openScratchpad();
    gScratchpadWindow.addEventListener("load", testFalsy, false);
  }, true);

  content.location = "data:text/html,<p>test falsy display() values in Scratchpad";
}

function testFalsy(sp)
{
  gScratchpadWindow.removeEventListener("load", testFalsy, false);

  let sp = gScratchpadWindow.Scratchpad;
  verifyFalsies(sp);
  
  sp.setBrowserContext();
  verifyFalsies(sp);

  finish();
}

function verifyFalsies(sp)
{
  sp.setText("undefined");
  sp.display();
  is(sp.selectedText, "/*\nundefined\n*/", "'undefined' is displayed");

  sp.setText("false");
  sp.display();
  is(sp.selectedText, "/*\nfalse\n*/", "'false' is displayed");

  sp.setText("0");
  sp.display();
  is(sp.selectedText, "/*\n0\n*/", "'0' is displayed");

  sp.setText("null");
  sp.display();
  is(sp.selectedText, "/*\nnull\n*/", "'null' is displayed");

  sp.setText("NaN");
  sp.display();
  is(sp.selectedText, "/*\nNaN\n*/", "'NaN' is displayed");

  sp.setText("''");
  sp.display();
  is(sp.selectedText, "/*\n\n*/", "empty string is displayed");
}

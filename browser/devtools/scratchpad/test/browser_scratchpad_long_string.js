



function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openScratchpad(runTests);
  }, true);

  content.location = "data:text/html;charset=utf8,<p>test long string in Scratchpad</p>";
}

function runTests()
{
  let sp = gScratchpadWindow.Scratchpad;

  sp.setText("'0'.repeat(10000)");

  sp.display().then(() => {
    is(sp.getText(), "'0'.repeat(10000)\n" +
                     "/*\n" + "0".repeat(10000) + "\n*/",
       "display()ing a long string works");
    finish();
  });
}

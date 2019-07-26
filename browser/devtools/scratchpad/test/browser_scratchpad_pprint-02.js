



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
  const tabsize = Services.prefs.getIntPref("devtools.editor.tabsize");
  Services.prefs.setIntPref("devtools.editor.tabsize", 6);
  const space = " ".repeat(6);

  const sp = sw.Scratchpad;
  sp.setText("function main() { console.log(5); }");
  sp.prettyPrint();
  const prettyText = sp.getText();
  ok(prettyText.contains(space));

  Services.prefs.setIntPref("devtools.editor.tabsize", tabsize);
  finish();
}





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
  sp.setText("function main() { console.log(5); }");
  sp.prettyPrint().then(() => {
    const prettyText = sp.getText();
    ok(prettyText.includes("\n"));
    finish();
  }).then(null, error => {
    ok(false, error);
  });
}

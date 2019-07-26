



function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openScratchpad(runTests);
  }, true);

  content.location = "data:text/html;charset=utf8,test display of values" +
                     " which can't be converted to string in Scratchpad";
}


function runTests()
{
  let sp = gScratchpadWindow.Scratchpad;
  sp.setText("Object.create(null);");

  sp.display().then(([, , aResult]) => {
    is(sp.getText(),
       "Object.create(null);\n" +
       "/*\nException: Cannot convert value to string.\n*/",
       "'Cannot convert value to string' comment is shown");

    finish();
  });
}

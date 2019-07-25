



const TESTCASE_URI = TEST_BASE + "simple.html";


function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();

  
  
  
  launchStyleEditorChrome(function (aChrome) {
    isnot(gBrowser.selectedBrowser.contentWindow.document.readyState, "complete",
          "content document is still loading");

    if (!aChrome.isContentAttached) {
      aChrome.addChromeListener({
        onContentAttach: run
      });
    } else {
      run(aChrome);
    }
  });

  content.location = TESTCASE_URI;
}

function run(aChrome)
{
  is(aChrome.contentWindow.document.readyState, "complete",
     "content document is complete");

  finish();
}

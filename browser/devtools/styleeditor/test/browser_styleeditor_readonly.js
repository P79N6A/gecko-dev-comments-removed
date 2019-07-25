



const TESTCASE_URI = TEST_BASE + "simple.html";


let gEditorAddedCount = 0;
let gEditorReadOnlyCount = 0;
let gChromeListener = {
  onEditorAdded: function (aChrome, aEditor) {
    gEditorAddedCount++;
    if (aEditor.readOnly) {
      gEditorReadOnlyCount++;
    }

    if (gEditorAddedCount == aChrome.editors.length) {
      

      is(gEditorReadOnlyCount, 0,
         "all editors are NOT read-only initially");

      
      executeSoon(function () {
        gBrowser.removeCurrentTab();
      });
    }
  },
  onContentDetach: function (aChrome) {
    
    run(aChrome);
  }
};

function test()
{
  waitForExplicitFinish();

  gBrowser.addTab(); 
  addTabAndLaunchStyleEditorChromeWhenLoaded(function (aChrome) {
    aChrome.addChromeListener(gChromeListener);
  });

  content.location = TESTCASE_URI;
}

function run(aChrome)
{
  let document = gChromeWindow.document;
  let disabledCount;
  let elements;

  disabledCount = 0;
  elements = document.querySelectorAll("button,input,select");
  for (let i = 0; i < elements.length; ++i) {
    if (elements[i].hasAttribute("disabled")) {
      disabledCount++;
    }
  }
  ok(elements.length && disabledCount == elements.length,
     "all buttons, input and select elements are disabled");

  disabledCount = 0;
  aChrome.editors.forEach(function (aEditor) {
    if (aEditor.readOnly) {
      disabledCount++;
    }
  });
  ok(aChrome.editors.length && disabledCount == aChrome.editors.length,
     "all editors are read-only");

  aChrome.removeChromeListener(gChromeListener);
  finish();
}

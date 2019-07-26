




const TESTCASE_URI = TEST_BASE_HTTP + "simple.html";

let tempScope = {};
Components.utils.import("resource://gre/modules/FileUtils.jsm", tempScope);
let FileUtils = tempScope.FileUtils;

const FILENAME = "styleeditor-import-test.css";
const SOURCE = "body{background:red;}";


function test()
{
  waitForExplicitFinish();

  addTabAndLaunchStyleEditorChromeWhenLoaded(function (aChrome) {
    aChrome.addChromeListener({
      onContentAttach: run,
      onEditorAdded: testEditorAdded
    });
    if (aChrome.isContentAttached) {
      run(aChrome);
    }
  });

  content.location = TESTCASE_URI;
}

function run(aChrome)
{
  is(aChrome.editors.length, 2,
     "there is 2 stylesheets initially");
}

function testImport(aChrome, aEditor)
{
  
  let file = FileUtils.getFile("ProfD", [FILENAME]);
  let ostream = FileUtils.openSafeFileOutputStream(file);
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let istream = converter.convertToInputStream(SOURCE);
  NetUtil.asyncCopy(istream, ostream, function (status) {
    FileUtils.closeSafeFileOutputStream(ostream);

    
    aChrome._mockImportFile = file;

    waitForFocus(function () {
      let document = gChromeWindow.document
      let importButton = document.querySelector(".style-editor-importButton");
      EventUtils.synthesizeMouseAtCenter(importButton, {}, gChromeWindow);
    }, gChromeWindow);
  });
}

let gAddedCount = 0;
function testEditorAdded(aChrome, aEditor)
{
  if (++gAddedCount == 2) {
    
    if (!aChrome.editors[0].sourceEditor) {
      aChrome.editors[0].addActionListener({
        onAttach: function () {
          testImport(aChrome);
        }
      });
    } else {
      testImport(aChrome);
    }
  }

  if (!aEditor.hasFlag("imported")) {
    return;
  }

  ok(!aEditor.hasFlag("inline"),
     "imported stylesheet does not have INLINE flag");
  ok(aEditor.savedFile,
     "imported stylesheet will be saved directly into the same file");
  is(aEditor.getFriendlyName(), FILENAME,
     "imported stylesheet has the same name as the filename");

  finish();
}

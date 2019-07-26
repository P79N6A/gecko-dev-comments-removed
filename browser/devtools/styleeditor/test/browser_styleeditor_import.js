




const TESTCASE_URI = TEST_BASE_HTTP + "simple.html";

let tempScope = {};
Components.utils.import("resource://gre/modules/FileUtils.jsm", tempScope);
let FileUtils = tempScope.FileUtils;

const FILENAME = "styleeditor-import-test.css";
const SOURCE = "body{background:red;}";


let gUI;

function test()
{
  waitForExplicitFinish();

  addTabAndOpenStyleEditor(function(panel) {
    gUI = panel.UI;
    gUI.on("editor-added", testEditorAdded);
  });

  content.location = TESTCASE_URI;
}

function testImport()
{
  
  let file = FileUtils.getFile("ProfD", [FILENAME]);
  let ostream = FileUtils.openSafeFileOutputStream(file);
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let istream = converter.convertToInputStream(SOURCE);
  NetUtil.asyncCopy(istream, ostream, function (status) {
    FileUtils.closeSafeFileOutputStream(ostream);

    
    gUI._mockImportFile = file;

    waitForFocus(function () {
      let document = gPanelWindow.document
      let importButton = document.querySelector(".style-editor-importButton");
      ok(importButton, "import button exists");

      EventUtils.synthesizeMouseAtCenter(importButton, {}, gPanelWindow);
    }, gPanelWindow);
  });
}

let gAddedCount = 0;
function testEditorAdded(aEvent, aEditor)
{
  if (++gAddedCount == 2) {
    
    gUI.editors[0].getSourceEditor().then(function() {
      testImport();
    });
  }

  if (!aEditor.savedFile) {
    return;
  }

  is(aEditor.savedFile.leafName, FILENAME,
     "imported stylesheet will be saved directly into the same file");
  is(aEditor.friendlyName, FILENAME,
     "imported stylesheet has the same name as the filename");

  gUI = null;
  finish();
}

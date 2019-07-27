


"use strict";




const TESTCASE_URI = TEST_BASE_HTTP + "simple.html";

let tempScope = {};
Components.utils.import("resource://gre/modules/FileUtils.jsm", tempScope);
let FileUtils = tempScope.FileUtils;

const FILENAME = "styleeditor-import-test.css";
const SOURCE = "body{background:red;}";

add_task(function* () {
  let { panel, ui } = yield openStyleEditorForURL(TESTCASE_URI);

  let added = ui.once("editor-added");
  importSheet(ui, panel.panelWindow);

  info("Waiting for editor to be added for the imported sheet.");
  let editor = yield added;

  is(editor.savedFile.leafName, FILENAME,
     "imported stylesheet will be saved directly into the same file");
  is(editor.friendlyName, FILENAME,
     "imported stylesheet has the same name as the filename");
});

function importSheet(ui, panelWindow)
{
  
  let file = FileUtils.getFile("ProfD", [FILENAME]);
  let ostream = FileUtils.openSafeFileOutputStream(file);
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let istream = converter.convertToInputStream(SOURCE);
  NetUtil.asyncCopy(istream, ostream, function (status) {
    FileUtils.closeSafeFileOutputStream(ostream);

    
    ui._mockImportFile = file;

    waitForFocus(function () {
      let document = panelWindow.document
      let importButton = document.querySelector(".style-editor-importButton");
      ok(importButton, "import button exists");

      EventUtils.synthesizeMouseAtCenter(importButton, {}, panelWindow);
    }, panelWindow);
  });
}

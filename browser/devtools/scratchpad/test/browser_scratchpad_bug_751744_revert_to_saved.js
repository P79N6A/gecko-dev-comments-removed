



let tempScope = {};
Cu.import("resource://gre/modules/NetUtil.jsm", tempScope);
Cu.import("resource://gre/modules/FileUtils.jsm", tempScope);
let NetUtil = tempScope.NetUtil;
let FileUtils = tempScope.FileUtils;


let gScratchpad;


let gFile;


let gFileName = "testFileForBug751744.tmp"



let gFileContent = "/* this file is already saved */\n" +
                   "function foo() { alert('bar') }";
let gLength = gFileContent.length;


let menu;

function startTest()
{
  gScratchpad = gScratchpadWindow.Scratchpad;
  menu = gScratchpadWindow.document.getElementById("sp-menu-revert");
  createAndLoadTemporaryFile();
}

function testAfterSaved() {
  
  ok(menu.hasAttribute("disabled"), "The revert menu entry is disabled.");

  
  gScratchpad.setText("\nfoo();", gLength, gLength);
  
  is(gScratchpad.getText(), gFileContent + "\nfoo();",
     "The text changed the first time.");

  
  ok(!menu.hasAttribute("disabled"),
     "The revert menu entry is enabled after changing text first time");

  
  gScratchpad.revertFile(testAfterRevert);
}

function testAfterRevert() {
  
  is(gScratchpad.getText(), gFileContent,
     "The text reverted back to original text.");
  
  ok(menu.hasAttribute("disabled"),
     "The revert menu entry is disabled after reverting.");

  
  gScratchpad.setText("\nalert(foo.toSource());", gLength, gLength);
  
  gScratchpad.saveFile(testAfterSecondSave);
}

function testAfterSecondSave() {
  
  ok(menu.hasAttribute("disabled"),
     "The revert menu entry is disabled after saving.");

  
  gScratchpad.setText("\nfoo();", gLength + 23, gLength + 23);

  
  ok(!menu.hasAttribute("disabled"),
     "The revert menu entry is enabled after changing text third time");

  
  gScratchpad.revertFile(testAfterSecondRevert);
}

function testAfterSecondRevert() {
  
  is(gScratchpad.getText(), gFileContent + "\nalert(foo.toSource());",
     "The text reverted back to the changed saved text.");
  
  ok(menu.hasAttribute("disabled"),
     "Revert menu entry is disabled after reverting to changed saved state.");
  gFile.remove(false);
  gFile = null;
  gScratchpad = null;
}

function createAndLoadTemporaryFile()
{
  
  gFile = FileUtils.getFile("TmpD", [gFileName]);
  gFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  
  let fout = Cc["@mozilla.org/network/file-output-stream;1"].
             createInstance(Ci.nsIFileOutputStream);
  fout.init(gFile.QueryInterface(Ci.nsILocalFile), 0x02 | 0x08 | 0x20,
            0644, fout.DEFER_OPEN);

  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let fileContentStream = converter.convertToInputStream(gFileContent);

  NetUtil.asyncCopy(fileContentStream, fout, tempFileSaved);
}

function tempFileSaved(aStatus)
{
  ok(Components.isSuccessCode(aStatus),
     "the temporary file was saved successfully");

  
  gScratchpad.setFilename(gFile.path);
  gScratchpad.importFromFile(gFile.QueryInterface(Ci.nsILocalFile),  true,
                             testAfterSaved);
}

function test()
{
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openScratchpad(startTest);
  }, true);

  content.location = "data:text/html,<p>test reverting to last saved state of" +
                     " a file </p>";
}

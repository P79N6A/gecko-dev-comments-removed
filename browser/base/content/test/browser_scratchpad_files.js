



Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");


let gScratchpadWindow;


let gScratchpad;


let gFile;


let gFileContent = "hello.world('bug636725');";

function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    gScratchpadWindow = Scratchpad.openScratchpad();
    gScratchpadWindow.addEventListener("load", runTests, false);
  }, true);

  content.location = "data:text/html,<p>test file open and save in Scratchpad";
}

function runTests()
{
  gScratchpadWindow.removeEventListener("load", arguments.callee, false);

  gScratchpad = gScratchpadWindow.Scratchpad;

  
  gFile = FileUtils.getFile("TmpD", ["fileForBug636725.tmp"]);
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

  
  gScratchpad.importFromFile(gFile.QueryInterface(Ci.nsILocalFile),  true,
                            fileImported);
}

function fileImported(aStatus, aFileContent)
{
  ok(Components.isSuccessCode(aStatus),
     "the temporary file was imported successfully with Scratchpad");

  is(aFileContent, gFileContent,
     "received data is correct");

  is(gScratchpad.textbox.value, gFileContent,
     "the textbox.value is correct");

  
  gFileContent += "// omg, saved!";
  gScratchpad.textbox.value = gFileContent;

  gScratchpad.exportToFile(gFile.QueryInterface(Ci.nsILocalFile), true, true,
                          fileExported);
}

function fileExported(aStatus)
{
  ok(Components.isSuccessCode(aStatus),
     "the temporary file was exported successfully with Scratchpad");

  let oldContent = gFileContent;

  
  gFileContent += "// omg, saved twice!";
  gScratchpad.textbox.value = gFileContent;

  let oldConfirm = gScratchpadWindow.confirm;
  let askedConfirmation = false;
  gScratchpadWindow.confirm = function() {
    askedConfirmation = true;
    return false;
  };

  gScratchpad.exportToFile(gFile.QueryInterface(Ci.nsILocalFile), false, true,
                          fileExported2);

  gScratchpadWindow.confirm = oldConfirm;

  ok(askedConfirmation, "exportToFile() asked for overwrite confirmation");

  gFileContent = oldContent;

  let channel = NetUtil.newChannel(gFile);
  channel.contentType = "application/javascript";

  
  NetUtil.asyncFetch(channel, fileRead);
}

function fileExported2()
{
  ok(false, "exportToFile() did not cancel file overwrite");
}

function fileRead(aInputStream, aStatus)
{
  ok(Components.isSuccessCode(aStatus),
     "the temporary file was read back successfully");

  let updatedContent =
    NetUtil.readInputStreamToString(aInputStream, aInputStream.available());;

  is(updatedContent, gFileContent, "file properly updated");

  
  gFile.remove(false);
  gFile = null;
  gScratchpad = null;
  gScratchpadWindow.close();
  gScratchpadWindow = null;
  gBrowser.removeCurrentTab();
  finish();
}

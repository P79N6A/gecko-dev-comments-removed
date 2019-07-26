



let tempScope = {};
Cu.import("resource://gre/modules/NetUtil.jsm", tempScope);
let NetUtil = tempScope.NetUtil;


let gScratchpad;


let gFile;


let gFileContent = "hello.world('bug636725');";

function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openScratchpad(runTests);
  }, true);

  content.location = "data:text/html,<p>test file open and save in Scratchpad";
}

function runTests()
{
  gScratchpad = gScratchpadWindow.Scratchpad;

  createTempFile("fileForBug636725.tmp", gFileContent, function(aStatus, aFile) {
    ok(Components.isSuccessCode(aStatus),
      "The temporary file was saved successfully");

      gFile = aFile;
      gScratchpad.importFromFile(gFile.QueryInterface(Ci.nsILocalFile), true,
        fileImported);
  });
}

function fileImported(aStatus, aFileContent)
{
  ok(Components.isSuccessCode(aStatus),
     "the temporary file was imported successfully with Scratchpad");

  is(aFileContent, gFileContent,
     "received data is correct");

  is(gScratchpad.getText(), gFileContent,
     "the editor content is correct");

  is(gScratchpad.dirty, false,
     "the editor marks imported file as saved");

  
  gFileContent += "// omg, saved!";
  gScratchpad.editor.setText(gFileContent);

  gScratchpad.exportToFile(gFile.QueryInterface(Ci.nsILocalFile), true, true,
                          fileExported);
}

function fileExported(aStatus)
{
  ok(Components.isSuccessCode(aStatus),
     "the temporary file was exported successfully with Scratchpad");

  let oldContent = gFileContent;

  
  gFileContent += "// omg, saved twice!";
  gScratchpad.editor.setText(gFileContent);

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
  finish();
}

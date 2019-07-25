



































var MockFilePicker = SpecialPowers.MockFilePicker;
MockFilePicker.reset();










function test() {
  waitForExplicitFinish();

  gBrowser.loadURI("http://mochi.test:8888/browser/toolkit/content/tests/browser/data/post_form_outer.sjs");

  registerCleanupFunction(function () {
    gBrowser.addTab();
    gBrowser.removeCurrentTab();
  });

  gBrowser.addEventListener("pageshow", function pageShown(event) {
    if (event.target.location == "about:blank")
      return;
    gBrowser.removeEventListener("pageshow", pageShown);

    
    
    gBrowser.addEventListener("DOMContentLoaded", handleOuterSubmit);
    gBrowser.contentDocument.getElementById("postForm").submit();
  });

  var framesLoaded = 0;
  var innerFrame;

  function handleOuterSubmit() {
    if (++framesLoaded < 2)
      return;

    gBrowser.removeEventListener("DOMContentLoaded", handleOuterSubmit);

    innerFrame = gBrowser.contentDocument.getElementById("innerFrame");

    
    gBrowser.addEventListener("DOMContentLoaded", handleInnerSubmit);
    innerFrame.contentDocument.getElementById("postForm").submit();
  }

  function handleInnerSubmit() {
    gBrowser.removeEventListener("DOMContentLoaded", handleInnerSubmit);

    
    var destDir = createTemporarySaveDirectory();
    var file = destDir.clone();
    file.append("no_default_file_name");
    MockFilePicker.returnFiles = [file];
    MockFilePicker.showCallback = function(fp) {
      MockFilePicker.filterIndex = 1; 
    };

    mockTransferCallback = onTransferComplete;
    mockTransferRegisterer.register();

    registerCleanupFunction(function () {
      mockTransferRegisterer.unregister();
      MockFilePicker.reset();
      destDir.remove(true);
    });

    var docToSave = innerFrame.contentDocument;
    
    
    internalSave(docToSave.location.href, docToSave, null, null,
                 docToSave.contentType, false, null, null,
                 docToSave.referrer ? makeURI(docToSave.referrer) : null,
                 false, null);
  }

  function onTransferComplete(downloadSuccess) {
    ok(downloadSuccess, "The inner frame should have been downloaded successfully");

    
    var file = MockFilePicker.returnFiles[0];
    var fileContents = readShortFile(file);

    
    is(fileContents.indexOf("inputfield=outer"), -1,
       "The saved inner frame does not contain outer POST data");

    
    isnot(fileContents.indexOf("inputfield=inner"), -1,
          "The saved inner frame was generated using the correct POST data");

    finish();
  }
}

Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("chrome://mochitests/content/browser/toolkit/content/tests/browser/common/mockTransfer.js",
                 this);

function createTemporarySaveDirectory() {
  var saveDir = Cc["@mozilla.org/file/directory_service;1"]
                  .getService(Ci.nsIProperties)
                  .get("TmpD", Ci.nsIFile);
  saveDir.append("testsavedir");
  if (!saveDir.exists())
    saveDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  return saveDir;
}










function readShortFile(aFile) {
  var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                      .createInstance(Ci.nsIFileInputStream);
  inputStream.init(aFile, -1, 0, 0);
  try {
    var scrInputStream = Cc["@mozilla.org/scriptableinputstream;1"]
                           .createInstance(Ci.nsIScriptableInputStream);
    scrInputStream.init(inputStream);
    try {
      
      return scrInputStream.read(1048576);
    }
    finally {
      
      
      scrInputStream.close();
    }
  }
  finally {
    
    
    inputStream.close();
  }
}

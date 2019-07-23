








































function test() {

  

  
  
  Components.classes["@mozilla.org/moz/jssubscript-loader;1"].
   getService(Components.interfaces.mozIJSSubScriptLoader).loadSubScript(
   "chrome://mochikit/content/browser/toolkit/content/tests/browser/common/_loadAll.js",
   this);

  

  const kBaseUrl =
        "http://localhost:8888/browser/toolkit/content/tests/browser/data/";

  function FramePostData_TestGenerator() {
    
    
    
    gBrowser.addEventListener("pageshow", testRunner.continueTest, false);
    gBrowser.loadURI(kBaseUrl + "post_form_outer.sjs");
    yield;
    gBrowser.removeEventListener("pageshow", testRunner.continueTest, false);

    try {
      
      
      gBrowser.addEventListener("DOMContentLoaded",
                                testRunner.continueAfterTwoEvents, false);
      try {
        gBrowser.contentDocument.getElementById("postForm").submit();
        yield;
      }
      finally {
        
        
        gBrowser.removeEventListener("DOMContentLoaded",
                                     testRunner.continueAfterTwoEvents, false);
      }

      
      var innerFrame = gBrowser.contentDocument.getElementById("innerFrame");

      
      gBrowser.addEventListener("DOMContentLoaded",
                                testRunner.continueTest, false);
      try {
        innerFrame.contentDocument.getElementById("postForm").submit();
        yield;
      }
      finally {
        
        
        gBrowser.removeEventListener("DOMContentLoaded",
                                     testRunner.continueTest, false);
      }

      
      var destDir = createTemporarySaveDirectory();
      try {
        
        mockFilePickerSettings.destDir = destDir;
        mockFilePickerSettings.filterIndex = 1; 
        callSaveWithMockObjects(function() {
          var docToSave = innerFrame.contentDocument;
          
          
          internalSave(docToSave.location.href, docToSave, null, null,
                       docToSave.contentType, false, null, null,
                       docToSave.referrer ? makeURI(docToSave.referrer) : null,
                       false, null);
        });

        
        var downloadSuccess = yield;
        if (!downloadSuccess)
          throw "Unexpected failure, the inner frame couldn't be saved!";

        
        var fileContents = readShortFile(mockFilePickerResults.selectedFile);

        
        const searchPattern = "inputfield=outer";
        ok(fileContents.indexOf(searchPattern) === -1,
           "The saved inner frame does not contain outer POST data");
      }
      finally {
        
        destDir.remove(true);
      }
    }
    finally {
      
      gBrowser.addTab().linkedBrowser.stop();
      gBrowser.removeCurrentTab();
    }
  }

  

  testRunner.runTest(FramePostData_TestGenerator);
}

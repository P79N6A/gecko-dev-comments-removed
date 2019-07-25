












































function test() {

  

  
  
  var rootDir = getRootDirectory(gTestPath);
  Components.classes["@mozilla.org/moz/jssubscript-loader;1"].
   getService(Components.interfaces.mozIJSSubScriptLoader).loadSubScript(
   rootDir + "common/_loadAll.js",
   this);

  

  const kBaseUrl =
        "http://mochi.test:8888/browser/toolkit/content/tests/browser/data/";

  function pageShown(event)
  {
    if (event.target.location != "about:blank")
      testRunner.continueTest();
  }

  function FramePostData_TestGenerator() {
    
    
    
    gBrowser.addEventListener("pageshow", pageShown, false);
    gBrowser.loadURI(kBaseUrl + "post_form_outer.sjs");
    yield;
    gBrowser.removeEventListener("pageshow", pageShown, false);

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

        
        ok(fileContents.indexOf("inputfield=outer") === -1,
           "The saved inner frame does not contain outer POST data");

        
        ok(fileContents.indexOf("inputfield=inner") > -1,
           "The saved inner frame was generated using the correct POST data");
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

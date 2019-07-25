






function test() {

  

  
  
  Components.classes["@mozilla.org/moz/jssubscript-loader;1"].
   getService(Components.interfaces.mozIJSSubScriptLoader).loadSubScript(
   "chrome://mochitests/content/browser/toolkit/content/tests/browser/common/_loadAll.js",
   this);

  

  const kBaseUrl =
        "http://mochi.test:8888/browser/browser/base/content/test/";

  function pageShown(event) {
    if (event.target.location != "about:blank")
      testRunner.continueTest();
  }

  function saveVideoAs_TestGenerator() {
    
    gBrowser.addEventListener("pageshow", pageShown, false);
    gBrowser.loadURI(kBaseUrl + "bug564387.html");
    yield;
    gBrowser.removeEventListener("pageshow", pageShown, false);

    
    SimpleTest.waitForFocus(testRunner.continueTest);
    yield;

    try {
      
      var video1 = gBrowser.contentDocument.getElementById("video1");

      
      
      document.addEventListener("popupshown", testRunner.continueTest, false);
      EventUtils.synthesizeMouseAtCenter(video1,
                                         { type: "contextmenu", button: 2 },
                                         gBrowser.contentWindow);
      yield;

      
      var destDir = createTemporarySaveDirectory();
      try {
        
        mockFilePickerSettings.destDir = destDir;
        mockFilePickerSettings.filterIndex = 1; 

        
        mockFilePickerRegisterer.register();
        try {
          
          mockTransferForContinuingRegisterer.register();
          try {
            
            var saveVideoCommand = document.getElementById("context-savevideo");
            saveVideoCommand.doCommand();

            
            document.removeEventListener("popupshown",
                                         testRunner.continueTest, false);
            
            document.getElementById("placesContext").hidePopup();

            
            var downloadSuccess = yield;
            if (!downloadSuccess)
              throw "Unexpected failure in downloading Video file!";
          }
          finally {
            
            mockTransferForContinuingRegisterer.unregister();
          }
        }
        finally {
          
          mockFilePickerRegisterer.unregister();
        }

        
        var fileName = mockFilePickerResults.selectedFile.leafName;

        is(fileName, "Bug564387-expectedName.ogv",
                     "Video File Name is correctly retrieved from Content-Disposition http header");
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

  
  testRunner.runTest(saveVideoAs_TestGenerator);
}

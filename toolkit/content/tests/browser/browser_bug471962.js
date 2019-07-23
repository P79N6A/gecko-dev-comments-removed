



































function test()
{
  const Cc = Components.classes;
  const Ci = Components.interfaces;
  const Cu = Components.utils;
  const Cr = Components.results;
  const Cm = Components.manager;

  Cu.import("resource://gre/modules/XPCOMUtils.jsm");

  

  var componentRegistrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);

  var originalFilePickerFactory;
  var mockFilePickerFactory;
  var destFile;

  
  
  
  const kFilePickerCID = "{bd57cee8-1dd1-11b2-9fe7-95cf4709aea3}";
  const kFilePickerContractID = "@mozilla.org/filepicker;1";
  const kFilePickerPossibleClassName = "File Picker";

  function registerMockFilePickerFactory() {
    
    
    
    var mockFilePicker = {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIFilePicker]),
      init: function(aParent, aTitle, aMode) { },
      appendFilters: function(aFilterMask) { },
      appendFilter: function(aTitle, aFilter) { },
      defaultString: "",
      defaultExtension: "",
      set filterIndex() { },
      get filterIndex() {
        return 1; 
      },
      displayDirectory: null,
      get file() {
        return destFile.clone();
      },
      get fileURL() {
        return Cc["@mozilla.org/network/io-service;1"].
               getService(Ci.nsIIOService).newFileURI(destFile);
      },
      get files() {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      show: function() {
        
        return (destFile.exists() ?
                Ci.nsIFilePicker.returnReplace :
                Ci.nsIFilePicker.returnOK);
      }
    };

    mockFilePickerFactory = {
      createInstance: function(aOuter, aIid) {
        if (aOuter != null)
          throw Cr.NS_ERROR_NO_AGGREGATION;
        return mockFilePicker.QueryInterface(aIid);
      }
    };

    
    originalFilePickerFactory = Cm.getClassObject(Cc[kFilePickerContractID],
                                                  Ci.nsIFactory);

    
    componentRegistrar.registerFactory(
      Components.ID(kFilePickerCID),
      "Mock File Picker Implementation",
      kFilePickerContractID,
      mockFilePickerFactory
    );
  }

  function unregisterMockFilePickerFactory() {
    
    componentRegistrar.unregisterFactory(
      Components.ID(kFilePickerCID),
      mockFilePickerFactory
    );

    
    componentRegistrar.registerFactory(
      Components.ID(kFilePickerCID),
      kFilePickerPossibleClassName,
      kFilePickerContractID,
      originalFilePickerFactory
    );
  }

  

  var downloadManager = Cc["@mozilla.org/download-manager;1"].
                        getService(Ci.nsIDownloadManager);

  var downloadListener = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIDownloadProgressListener]),
    document: null,
    onDownloadStateChange: function(aState, aDownload) {
      switch (aDownload.state) {
        
        case Ci.nsIDownloadManager.DOWNLOAD_FINISHED:
          onDownloadFinished(true);
          break;

        
        case Ci.nsIDownloadManager.DOWNLOAD_DIRTY:
        case Ci.nsIDownloadManager.DOWNLOAD_FAILED:
        case Ci.nsIDownloadManager.DOWNLOAD_CANCELED:
          onDownloadFinished(false);
          break;
      }
    },
    onStateChange: function(aWebProgress, aRequest, aState, aStatus,
     aDownload) { },
    onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress,
     aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress, aDownload) { },
    onSecurityChange: function(aWebProgress, aRequest, aState) { }
  }

  

  var mainPrefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch);

  const kShowWhenStartingPref = "browser.download.manager.showWhenStarting";
  const kRetentionPref = "browser.download.manager.retention";

  function tweakDownloadManagerPrefs() {
    
    mainPrefBranch.setBoolPref(kShowWhenStartingPref, false);
    
    mainPrefBranch.setIntPref(kRetentionPref, 0);
  }

  function restoreDownloadManagerPrefs() {
    mainPrefBranch.clearUserPref(kShowWhenStartingPref);
    mainPrefBranch.clearUserPref(kRetentionPref);
  }

  

  var innerFrame;

  function startTest() {
    waitForExplicitFinish();

    
    gBrowser.addEventListener("pageshow", onPageShow, false);
    gBrowser.loadURI("http://localhost:8888/browser/toolkit/content/tests/browser/bug471962_testpage_outer.sjs");
  }

  function onPageShow() {
    gBrowser.removeEventListener("pageshow", onPageShow, false);

    
    
    gBrowser.addEventListener("DOMContentLoaded", waitForTwoReloads, false);
    gBrowser.contentDocument.getElementById("postForm").submit();
  }

  var isFirstReload = true;

  function waitForTwoReloads() {
    
    if (isFirstReload) {
      isFirstReload = false;
      return;
    }

    
    gBrowser.removeEventListener("DOMContentLoaded", waitForTwoReloads, false);

    
    innerFrame = gBrowser.contentDocument.getElementById("innerFrame");

    
    gBrowser.addEventListener("DOMContentLoaded", onInnerSubmitted, false);
    innerFrame.contentDocument.getElementById("postForm").submit();
  }

  function onInnerSubmitted() {
    gBrowser.removeEventListener("DOMContentLoaded", onInnerSubmitted, false);

    
    destFile = Cc["@mozilla.org/file/directory_service;1"].
     getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
    destFile.append("testsave_bug471962.html");

    
    downloadManager.addListener(downloadListener);

    
    tweakDownloadManagerPrefs();

    
    
    
    registerMockFilePickerFactory();
    var docToSave = innerFrame.contentDocument;
    
    internalSave(docToSave.location.href, docToSave, null, null,
                 docToSave.contentType, false, null, null,
                 docToSave.referrer ? makeURI(docToSave.referrer) : null,
                 false, null);
    unregisterMockFilePickerFactory();
  }

  function onDownloadFinished(aSuccess) {
    downloadManager.removeListener(downloadListener);
    restoreDownloadManagerPrefs();

    
    if (!aSuccess) {
      ok(false, "Unexpected failure, the inner frame couldn't be saved!");
      finish();
      return;
    }

    
    var inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                      createInstance(Ci.nsIFileInputStream);
    inputStream.init(destFile, -1, 0, 0);
    var scrInputStream = Cc["@mozilla.org/scriptableinputstream;1"].
                         createInstance(Ci.nsIScriptableInputStream);
    scrInputStream.init(inputStream);
    var fileContents = scrInputStream.
                       read(1048576); 
    scrInputStream.close();
    inputStream.close();

    
    const searchPattern = "inputfield=outer";
    ok(fileContents.indexOf(searchPattern) === -1,
       "The saved inner frame does not contain outer POST data");

    
    gBrowser.addTab();
    gBrowser.removeCurrentTab();

    
    destFile.remove(false);
    finish();
  }

  
  startTest();
}

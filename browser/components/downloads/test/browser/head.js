











XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");
const nsIDM = Ci.nsIDownloadManager;

let gTestTargetFile = FileUtils.getFile("TmpD", ["dm-ui-test.file"]);
gTestTargetFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
registerCleanupFunction(function () {
  gTestTargetFile.remove(false);
});




let gDownloadRowTemplate = {
  name: "test-download.txt",
  source: "http://www.example.com/test-download.txt",
  target: NetUtil.newURI(gTestTargetFile).spec,
  startTime: 1180493839859230,
  endTime: 1180493839859234,
  state: nsIDM.DOWNLOAD_FINISHED,
  currBytes: 0,
  maxBytes: -1,
  preferredAction: 0,
  autoResume: 0
};





function test()
{
  testRunner.runTest(this.gen_test);
}







var testRunner = {
  _testIterator: null,
  _lastEventResult: undefined,
  _testRunning: false,
  _eventRaised: false,

  

  










  runTest: function TR_runTest(aGenerator) {
    waitForExplicitFinish();
    testRunner._testIterator = aGenerator();
    testRunner.continueTest();
  },

  





  continueTest: function TR_continueTest(aEventResult) {
    
    testRunner._lastEventResult = aEventResult;

    
    if (testRunner._testRunning) {
      testRunner._eventRaised = true;
      return;
    }

    
    testRunner._testRunning = true;
    try {
      do {
        
        
        testRunner._eventRaised = false;
        testRunner._testIterator.send(testRunner._lastEventResult);
      } while (testRunner._eventRaised);
    }
    catch (e) {
      
      
      
      if (!(e instanceof StopIteration))
        ok(false, e);
      
      finish();
    }

    
    testRunner._testRunning = false;
  }
};












function gen_resetState()
{
  let statement = Services.downloads.DBConnection.createAsyncStatement(
                  "DELETE FROM moz_downloads");
  try {
    statement.executeAsync({
      handleResult: function(aResultSet) { },
      handleError: function(aError)
      {
        Cu.reportError(aError);
      },
      handleCompletion: function(aReason)
      {
        testRunner.continueTest();
      }
    });
    yield;
  } finally {
    statement.finalize();
  }

  
  Services.prefs.clearUserPref("browser.download.panel.shown");

  
  DownloadsCommon.data.clear();
  DownloadsCommon.data._loadState = DownloadsCommon.data.kLoadNone;
  DownloadsPanel.hidePanel();

  
  waitForFocus(testRunner.continueTest);
  yield;
}

function gen_addDownloadRows(aDataRows)
{
  let columnNames = Object.keys(gDownloadRowTemplate).join(", ");
  let parameterNames = Object.keys(gDownloadRowTemplate)
                             .map(function(n) ":" + n)
                             .join(", ");
  let statement = Services.downloads.DBConnection.createAsyncStatement(
                  "INSERT INTO moz_downloads (" + columnNames +
                                    ") VALUES(" + parameterNames + ")");
  try {
    
    for (let i = aDataRows.length - 1; i >= 0; i--) {
      let dataRow = aDataRows[i];

      
      for (let columnName in gDownloadRowTemplate) {
        if (!(columnName in dataRow)) {
          
          
          dataRow[columnName] = gDownloadRowTemplate[columnName];
        }
        statement.params[columnName] = dataRow[columnName];
      }

      
      statement.executeAsync({
        handleResult: function(aResultSet) { },
        handleError: function(aError)
        {
          Cu.reportError(aError);
        },
        handleCompletion: function(aReason)
        {
          testRunner.continueTest();
        }
      });
      yield;

      
      
      gDownloadRowTemplate.endTime++;
    }
  } finally {
    statement.finalize();
  }
}

function gen_openPanel(aData)
{
  
  let originalOnViewLoadCompleted = DownloadsPanel.onViewLoadCompleted;
  DownloadsPanel.onViewLoadCompleted = function () {
    DownloadsPanel.onViewLoadCompleted = originalOnViewLoadCompleted;
    originalOnViewLoadCompleted.apply(this);
    testRunner.continueTest();
  };

  
  DownloadsCommon.data.ensurePersistentDataLoaded(false);

  
  waitForFocus(testRunner.continueTest);
  yield;

  
  DownloadsPanel.showPanel();
  yield;
}









function waitFor(aSeconds)
{
  setTimeout(function() {
    testRunner.continueTest();
  }, aSeconds * 1000);
}













function prepareForPanelOpen()
{
  
  let originalOnPopupShown = DownloadsPanel.onPopupShown;
  DownloadsPanel.onPopupShown = function (aEvent) {
    DownloadsPanel.onPopupShown = originalOnPopupShown;
    DownloadsPanel.onPopupShown.apply(this, [aEvent]);
    testRunner.continueTest();
  };
}

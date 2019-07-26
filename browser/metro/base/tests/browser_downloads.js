




"use strict";









XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");

const nsIDM = Ci.nsIDownloadManager;




var { spawn } = Task;

function equalStrings(){
  let ref = ""+arguments[0];
  for (let i=1; i<arguments.length; i++){
    if (ref !== ""+arguments[i]) {
      info("equalStrings failure: " + ref + " != " + arguments[i]);
      return false
    }
  }
  return true;
}

function equalNumbers(){
  let ref = Number(arguments[0]);
  for (let i=1; i<arguments.length; i++){
    if (ref !== Number(arguments[i])) return false;
    if (ref !== Number(arguments[i])) {
      info("equalNumbers failure: " + ref + " != " + Number(arguments[i]));
      return false
    }
  }
  return true;
}

function waitForMs(aDelay) {
  let deferred = Promise.defer();
  let timerID = setTimeout(function(){
    deferred.resolve(true);
  }, aDelay || 0);
  return deferred.promise;
}

function getPromisedDbResult(aStatement) {
  let dbConnection = Downloads.manager.DBConnection;
  let statement = ("string" == typeof aStatement) ?
        dbConnection.createAsyncStatement(
          aStatement
        ) : aStatement;

  let deferred = Promise.defer(),
      resultRows = [],
      err = null;
  try {
    statement.executeAsync({
      handleResult: function(aResultSet) {
        let row;
        if(!aResultSet) {
          return;
        }
        while ((row = aResultSet.getNextRow())){
          resultRows.push(row);
        }
      },
      handleError: function(aError) {
        Cu.reportError(aError);
        err = aError;
      },
      handleCompletion: function(){
        if (err) {
          deferred.reject(err);
        } else {
          deferred.resolve(resultRows);
        }
     }
    });
  } finally {
    statement.finalize();
  }
  return deferred.promise;
}

let gTestTargetFile = FileUtils.getFile("TmpD", ["dm-ui-test.file"]);
gTestTargetFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
registerCleanupFunction(function () {
  gTestTargetFile.remove(false);
  PanelUI.hide();
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




function test() {
  PanelUI.show("downloads-container");
  runTests();
}



function resetDownloads(){
  var defd = Promise.defer();
  
    

  let promisedResult = getPromisedDbResult(
    "DELETE FROM moz_downloads"
  );
  return promisedResult.then(function(aResult){
    
    

    
    let dlMgr = Downloads.manager;
    let dlsToRemove = [];
    
    dlMgr.cleanUp();
    dlMgr.cleanUpPrivate();

    
    for (let dlsEnum of [dlMgr.activeDownloads, dlMgr.activePrivateDownloads]) {
      while (dlsEnum.hasMoreElements()) {
        dlsToRemove.push(dlsEnum.next());
      }
    }
    
    dlsToRemove.forEach(function (dl) {
      dl.remove();
    });
  });
}

function addDownloadRow(aDataRow) {
  let deferredInsert = Promise.defer();
  let dataRow = aDataRow;

  let dm = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
  let db = dm.DBConnection;

  let columnNames = Object.keys(gDownloadRowTemplate).join(", ");
  let parameterNames = Object.keys(gDownloadRowTemplate)
                             .map(function(n) ":" + n)
                             .join(", ");

  let statement = db.createAsyncStatement(
                  "INSERT INTO moz_downloads (" + columnNames +
                  ", guid) VALUES(" + parameterNames + ", GENERATE_GUID())");

  
  for (let columnName in gDownloadRowTemplate) {
    if (!(columnName in dataRow)) {
      
      
      dataRow[columnName] = gDownloadRowTemplate[columnName];
    }
    statement.params[columnName] = dataRow[columnName];
  }

  
  let promisedDownloads = getPromisedDbResult(
    statement
  );
  yield promisedDownloads.then(function(){
      let newItemId = db.lastInsertRowID;
      let download = dm.getDownload(newItemId);
      deferredInsert.resolve(download);
  });
}

function gen_addDownloadRows(aDataRows){
  if (!aDataRows.length) {
    yield;
  }

  try {
    
    for (let i = aDataRows.length - 1; i >= 0; i--) {
      let dataRow = aDataRows[i];
      let download = yield addDownloadRow(dataRow);

      
      
      
      gDownloadRowTemplate.startTime++;
      gDownloadRowTemplate.endTime++;
    }
  } finally {
    info("gen_addDownloadRows, finally");
  }
}











gTests.push({
  desc: "UI sanity check",
  run: function(){

    ok(document.getElementById('downloads-list'), "Downloads panel grid is present");
    ok(DownloadsPanelView, "Downloads panel object is present");

    PanelUI.show('downloads-container');
    ok(DownloadsPanelView.visible, "Downloads panel is visible after being shown");
  }
});

gTests.push({
  desc: "zero downloads",
  run: function () {

    yield resetDownloads();

    let downloadslist = document.getElementById("downloads-list");

    
    
    let isReady = waitForEvent(downloadslist, "DownloadsReady", 2000);
    
    DownloadsPanelView._view.getDownloads();

    yield isReady;

    if (!isReady || isReady instanceof Error){
      ok(false, "DownloadsReady event never fired");
    }

    let count = downloadslist.children.length;
    is(count, 0, "Zero items in grid view with empty downloads db");
  }
});





gTests.push({
  desc: "Show downloads",
  run: function(){
    
    let DownloadData = [
      { endTime: 1180493839859239, state: nsIDM.DOWNLOAD_NOTSTARTED },
      { endTime: 1180493839859238, state: nsIDM.DOWNLOAD_DOWNLOADING },
      { endTime: 1180493839859237, state: nsIDM.DOWNLOAD_PAUSED },
      { endTime: 1180493839859236, state: nsIDM.DOWNLOAD_SCANNING },
      { endTime: 1180493839859235, state: nsIDM.DOWNLOAD_QUEUED },
      { endTime: 1180493839859234, state: nsIDM.DOWNLOAD_FINISHED },
      { endTime: 1180493839859233, state: nsIDM.DOWNLOAD_FAILED },
      { endTime: 1180493839859232, state: nsIDM.DOWNLOAD_CANCELED },
      { endTime: 1180493839859231, state: nsIDM.DOWNLOAD_BLOCKED_PARENTAL },
      { endTime: 1180493839859230, state: nsIDM.DOWNLOAD_DIRTY },
      { endTime: 1180493839859229, state: nsIDM.DOWNLOAD_BLOCKED_POLICY },
    ];

    yield resetDownloads();
    DownloadsPanelView._view.getDownloads();

    

    try {
      
      
      yield spawn( gen_addDownloadRows( DownloadData ) );

      
      let downloadslist = document.getElementById("downloads-list");
      
      
      let isReady = waitForEvent(downloadslist, "DownloadsReady", 2000);

      
      DownloadsPanelView._view.getDownloads();

      yield isReady;

      if (!isReady || isReady instanceof Error){
        ok(false, "DownloadsReady event never fired");
      }

      is(downloadslist.children.length, DownloadData.length,
         "There is the correct number of richlistitems");

      for (let i = 0; i < downloadslist.children.length; i++) {
        let element = downloadslist.children[i];
        let id = element.getAttribute("downloadId");
        let dataItem = Downloads.manager.getDownload(id); 

        ok( equalStrings(
              element.getAttribute("name"),
              dataItem.displayName,
              DownloadData[i].name
            ), "Download names match up");

        ok( equalNumbers(
                element.getAttribute("state"),
                dataItem.state,
                DownloadData[i].state
            ), "Download states match up");

        ok( equalStrings(
              element.getAttribute("target"),
              dataItem.target.spec,
              DownloadData[i].target
            ), "Download targets match up");

        if (DownloadData[i].source && dataItem.referrer){
          ok( equalStrings(
                dataItem.referrer.spec,
                DownloadData[i].source
              ), "Download sources match up");
        }
      }
    } catch(e) {
      info("Show downloads, some error: " + e);
    }
    finally {
      
      DownloadsPanelView._view.clearDownloads();
      yield resetDownloads();
    }
  }
});




gTests.push({
  desc: "Remove downloads",
  run: function(){
    
    let DownloadData = [
      { endTime: 1180493839859239, state: nsIDM.DOWNLOAD_FINISHED },
      { endTime: 1180493839859238, state: nsIDM.DOWNLOAD_FINISHED },
      { endTime: 1180493839859237, state: nsIDM.DOWNLOAD_FINISHED }
    ];

    yield resetDownloads();
    DownloadsPanelView._view.getDownloads();

    try {
      
      yield spawn( gen_addDownloadRows( DownloadData ) );

      
      let downloadslist = document.getElementById("downloads-list");
      
      
      let isReady = waitForEvent(downloadslist, "DownloadsReady", 2000);
      
      DownloadsPanelView._view.getDownloads();

      yield isReady;

      if (!isReady || isReady instanceof Error){
        is(false, "DownloadsReady event never fired");
      }

      let downloadRows = null,
          promisedDownloads;
      
      promisedDownloads = getPromisedDbResult(
        "SELECT guid "
      + "FROM moz_downloads "
      + "ORDER BY startTime DESC"
      ).then(function(aRows){
        downloadRows = aRows;
      }, function(aError){
        throw aError;
      });
      yield promisedDownloads;

      is(downloadRows.length, 3, "Correct number of downloads in the db before removal");

      
      let itemNode = downloadslist.children[0];
      let id = itemNode.getAttribute("downloadId");
      
      let download = Downloads.manager.getDownload( id );
      let file = download.targetFile;
      ok(file && file.exists());

      Downloads.manager.removeDownload( id );
      
      yield waitForMs(0);

      
      downloadRows = null;
      promisedDownloads = getPromisedDbResult(
        "SELECT guid "
      + "FROM moz_downloads "
      + "ORDER BY startTime DESC"
      ).then(function(aRows){
        downloadRows = aRows;
      }, function(aError){
        throw aError;
      });
      yield promisedDownloads;

      is(downloadRows.length, 2, "Correct number of downloads in the db after removal");

      is(2, downloadslist.children.length,
         "Removing a download updates the items list");
      ok(file && file.exists(), "File still exists after download removal");

    } catch(e) {
      info("Remove downloads, some error: " + e);
    }
    finally {
      
      DownloadsPanelView._view.clearDownloads();
      yield resetDownloads();
    }
  }
});



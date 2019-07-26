




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

function getPromisedDbResult(aStatement) {
  let dbConnection = MetroDownloadsView.manager.DBConnection;
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
  runTests();
}



function resetDownloads(){
  
  

  let promisedResult = getPromisedDbResult(
    "DELETE FROM moz_downloads"
  );
  return promisedResult.then(function(aResult){
    
    

    
    let dlMgr = MetroDownloadsView.manager;
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
    yield null;
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
  desc: "zero downloads",
  run: function () {
    yield resetDownloads();
    todo(false, "Test there are no visible notifications with an empty db.");
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
      { endTime: 1180493839859229, state: nsIDM.DOWNLOAD_BLOCKED_POLICY }
    ];

    yield resetDownloads();

    try {
      
      
      yield spawn( gen_addDownloadRows( DownloadData ) );

      todo( false, "Check that MetroDownloadsView._progressNotificationInfo and MetroDownloadsView._downloadCount \
        have the correct length (DownloadData.length) \
        May also test that the correct notifications show up for various states.");

      todo(false, "Iterate through download objects in MetroDownloadsView._progressNotificationInfo \
        and confirm that the downloads they refer to are the same as those in \
        DownloadData.");
    } catch(e) {
      info("Show downloads, some error: " + e);
    }
    finally {
      
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

    try {
      
      yield spawn( gen_addDownloadRows( DownloadData ) );

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

      todo(false, "Get some download from MetroDownloadsView._progressNotificationInfo, \
        confirm that its file exists, then remove it.");

      
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

      todo(false, "confirm that the removed download is no longer in the database \
        and its file no longer exists.");

    } catch(e) {
      info("Remove downloads, some error: " + e);
    }
    finally {
      
      yield resetDownloads();
    }
  }
});




gTests.push({
  desc: "Cancel/Abort Downloads",
  run: function(){
    todo(false, "Ensure that a cancelled/aborted download is in the correct state \
      including correct values for state variables (e.g. _downloadCount, _downloadsInProgress) \
      and the existence of the downloaded file.");
  }
});
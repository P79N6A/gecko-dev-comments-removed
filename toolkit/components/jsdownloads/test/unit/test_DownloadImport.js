






"use strict";




XPCOMUtils.defineLazyModuleGetter(this, "Sqlite",
                                  "resource://gre/modules/Sqlite.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadImport",
                                  "resource://gre/modules/DownloadImport.jsm");


const DOWNLOAD_NOTSTARTED = -1;
const DOWNLOAD_DOWNLOADING = 0;
const DOWNLOAD_PAUSED = 4;
const DOWNLOAD_QUEUED = 5;


const DOWNLOAD_FAILED = 2;
const DOWNLOAD_CANCELED = 3;
const DOWNLOAD_BLOCKED_PARENTAL = 6;
const DOWNLOAD_SCANNING = 7;
const DOWNLOAD_DIRTY = 8;
const DOWNLOAD_BLOCKED_POLICY = 9;







const TEST_DATA_REPLACEMENT = "-changed- ";
const TEST_DATA_TAINTED = TEST_DATA_REPLACEMENT +
                          TEST_DATA_SHORT.substr(TEST_DATA_REPLACEMENT.length);
const TEST_DATA_LENGTH = TEST_DATA_SHORT.length;



const TEST_DATA_PARTIAL_LENGTH = TEST_DATA_REPLACEMENT.length;




const MAXBYTES_IN_DB = TEST_DATA_LENGTH - 10;

let gDownloadsRowToImport;
let gDownloadsRowNonImportable;














function promiseEmptyDatabaseConnection({aPath, aSchemaVersion}) {
  return Task.spawn(function () {
    let connection = yield Sqlite.openConnection({ path: aPath });

    yield connection.execute("CREATE TABLE moz_downloads ("
                             + "id INTEGER PRIMARY KEY,"
                             + "name TEXT,"
                             + "source TEXT,"
                             + "target TEXT,"
                             + "tempPath TEXT,"
                             + "startTime INTEGER,"
                             + "endTime INTEGER,"
                             + "state INTEGER,"
                             + "referrer TEXT,"
                             + "entityID TEXT,"
                             + "currBytes INTEGER NOT NULL DEFAULT 0,"
                             + "maxBytes INTEGER NOT NULL DEFAULT -1,"
                             + "mimeType TEXT,"
                             + "preferredApplication TEXT,"
                             + "preferredAction INTEGER NOT NULL DEFAULT 0,"
                             + "autoResume INTEGER NOT NULL DEFAULT 0,"
                             + "guid TEXT)");

    yield connection.setSchemaVersion(aSchemaVersion);

    throw new Task.Result(connection);
  });
}














function promiseInsertRow(aConnection, aDownloadRow) {
  
  
  
  
  let values = [
    aDownloadRow.source, aDownloadRow.target, aDownloadRow.tempPath,
    aDownloadRow.startTime.getTime() * 1000, aDownloadRow.state,
    aDownloadRow.referrer, aDownloadRow.entityID, aDownloadRow.maxBytes,
    aDownloadRow.mimeType, aDownloadRow.preferredApplication,
    aDownloadRow.preferredAction, aDownloadRow.autoResume
  ];

  return aConnection.execute("INSERT INTO moz_downloads ("
                            + "name, source, target, tempPath, startTime,"
                            + "endTime, state, referrer, entityID, currBytes,"
                            + "maxBytes, mimeType, preferredApplication,"
                            + "preferredAction, autoResume, guid)"
                            + "VALUES ("
                            + "'', ?, ?, ?, ?, " 
                            + "0, ?, ?, ?, 0, "  
                            + " ?, ?, ?, "       
                            + " ?, ?, '')",      
                            values);
}












function promiseTableCount(aConnection) {
  return aConnection.execute("SELECT COUNT(*) FROM moz_downloads")
                    .then(res => res[0].getResultByName("COUNT(*)"))
                    .then(null, Cu.reportError);
}












function promiseEntityID(aUrl) {
  let deferred = Promise.defer();
  let entityID = "";
  let channel = NetUtil.newChannel2(NetUtil.newURI(aUrl),
                                    null,
                                    null,
                                    null,      
                                    Services.scriptSecurityManager.getSystemPrincipal(),
                                    null,      
                                    Ci.nsILoadInfo.SEC_NORMAL,
                                    Ci.nsIContentPolicy.TYPE_OTHER);

  channel.asyncOpen({
    onStartRequest: function (aRequest) {
      if (aRequest instanceof Ci.nsIResumableChannel) {
        entityID = aRequest.entityID;
      }
      aRequest.cancel(Cr.NS_BINDING_ABORTED);
    },

    onStopRequest: function (aRequest, aContext, aStatusCode) {
      if (aStatusCode == Cr.NS_BINDING_ABORTED) {
        deferred.resolve(entityID);
      } else {
        deferred.reject("Unexpected status code received");
      }
    },

    onDataAvailable: function () {}
  }, null);

  return deferred.promise;
}











function getDownloadTarget(aLeafName) {
  return NetUtil.newURI(getTempFile(aLeafName)).spec;
}


















function getPartialFile(aLeafName, aTainted = false) {
  let tempDownload = getTempFile(aLeafName);
  let partialContent = aTainted
                     ? TEST_DATA_TAINTED.substr(0, TEST_DATA_PARTIAL_LENGTH)
                     : TEST_DATA_SHORT.substr(0, TEST_DATA_PARTIAL_LENGTH);

  return OS.File.writeAtomic(tempDownload.path, partialContent,
                             { tmpPath: tempDownload.path + ".tmp",
                               flush: true })
                .then(() => tempDownload.path);
}












function getStartTime(aOffset) {
  return new Date(1000000 + (aOffset * 10000));
}
















function checkDownload(aDownload, aDownloadRow) {
  return Task.spawn(function() {
    do_check_eq(aDownload.source.url, aDownloadRow.source);
    do_check_eq(aDownload.source.referrer, aDownloadRow.referrer);

    do_check_eq(aDownload.target.path,
                NetUtil.newURI(aDownloadRow.target)
                       .QueryInterface(Ci.nsIFileURL).file.path);

    do_check_eq(aDownload.target.partFilePath, aDownloadRow.tempPath);

    if (aDownloadRow.expectedResume) {
      do_check_true(!aDownload.stopped || aDownload.succeeded);
      yield promiseDownloadStopped(aDownload);

      do_check_true(aDownload.succeeded);
      do_check_eq(aDownload.progress, 100);
      
      
      do_check_neq(aDownload.startTime.toJSON(),
                   aDownloadRow.startTime.toJSON());
    } else {
      do_check_false(aDownload.succeeded);
      do_check_eq(aDownload.startTime.toJSON(),
                  aDownloadRow.startTime.toJSON());
    }

    do_check_eq(aDownload.stopped, true);

    let serializedSaver = aDownload.saver.toSerializable();
    if (typeof(serializedSaver) == "object") {
      do_check_eq(serializedSaver.type, "copy");
    } else {
      do_check_eq(serializedSaver, "copy");
    }

    if (aDownloadRow.entityID) {
      do_check_eq(aDownload.saver.entityID, aDownloadRow.entityID);
    }

    do_check_eq(aDownload.currentBytes, aDownloadRow.expectedCurrentBytes);
    do_check_eq(aDownload.totalBytes, aDownloadRow.expectedTotalBytes);

    if (aDownloadRow.expectedContent) {
      let fileToCheck = aDownloadRow.expectedResume
                        ? aDownload.target.path
                        : aDownload.target.partFilePath;
      yield promiseVerifyContents(fileToCheck, aDownloadRow.expectedContent);
    }

    do_check_eq(aDownload.contentType, aDownloadRow.expectedContentType);
    do_check_eq(aDownload.launcherPath, aDownloadRow.preferredApplication);

    do_check_eq(aDownload.launchWhenSucceeded,
                aDownloadRow.preferredAction != Ci.nsIMIMEInfo.saveToDisk);
  });
}








add_task(function prepareDownloadsToImport() {

  let sourceUrl = httpUrl("source.txt");
  let sourceEntityId = yield promiseEntityID(sourceUrl);

  gDownloadsRowToImport = [
    
    
    
    
    
    {
      source: sourceUrl,
      target: getDownloadTarget("inprogress1.txt"),
      tempPath: yield getPartialFile("inprogress1.txt.part", true),
      startTime: getStartTime(1),
      state: DOWNLOAD_PAUSED,
      referrer: httpUrl("referrer1"),
      entityID: sourceEntityId,
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "mimeType1",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication1",
      autoResume: 1,

      
      
      
      
      
      expectedCurrentBytes: TEST_DATA_LENGTH,
      expectedTotalBytes: TEST_DATA_LENGTH,
      expectedResume: true,
      expectedContentType: "text/plain",
      expectedContent: TEST_DATA_TAINTED,
    },

    
    
    
    
    
    {
      source: sourceUrl,
      target: getDownloadTarget("inprogress2.txt"),
      tempPath: yield getPartialFile("inprogress2.txt.part", true),
      startTime: getStartTime(2),
      state: DOWNLOAD_PAUSED,
      referrer: httpUrl("referrer2"),
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "mimeType2",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication2",
      autoResume: 1,

      expectedCurrentBytes: TEST_DATA_LENGTH,
      expectedTotalBytes: TEST_DATA_LENGTH,
      expectedResume: true,
      expectedContentType: "text/plain",
      expectedContent: TEST_DATA_SHORT
    },

    
    {
      source: sourceUrl,
      target: getDownloadTarget("inprogress3.txt"),
      tempPath: yield getPartialFile("inprogress3.txt.part"),
      startTime: getStartTime(3),
      state: DOWNLOAD_PAUSED,
      referrer: httpUrl("referrer3"),
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "mimeType3",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication3",
      autoResume: 0,

      
      
      
      expectedCurrentBytes: TEST_DATA_PARTIAL_LENGTH,
      expectedTotalBytes: MAXBYTES_IN_DB,
      expectedResume: false,
      expectedContentType: "mimeType3",
      expectedContent: TEST_DATA_SHORT.substr(0, TEST_DATA_PARTIAL_LENGTH),
    },

    
    {
      source: sourceUrl,
      target: getDownloadTarget("inprogress4.txt"),
      tempPath: "",
      startTime: getStartTime(4),
      state: DOWNLOAD_PAUSED,
      referrer: httpUrl("referrer4"),
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "text/plain",
      preferredAction: Ci.nsIMIMEInfo.useHelperApp,
      preferredApplication: "prerredApplication4",
      autoResume: 1,

      expectedCurrentBytes: TEST_DATA_LENGTH,
      expectedTotalBytes: TEST_DATA_LENGTH,
      expectedResume: true,
      expectedContentType: "text/plain",
      expectedContent: TEST_DATA_SHORT
    },

    
    {
      source: sourceUrl,
      target: getDownloadTarget("inprogress5.txt"),
      tempPath: "",
      startTime: getStartTime(5),
      state: DOWNLOAD_PAUSED,
      referrer: httpUrl("referrer4"),
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "text/plain",
      preferredAction: Ci.nsIMIMEInfo.useSystemDefault,
      preferredApplication: "prerredApplication5",
      autoResume: 0,

      expectedCurrentBytes: 0,
      expectedTotalBytes: MAXBYTES_IN_DB,
      expectedResume: false,
      expectedContentType: "text/plain",
    },

    
    
    {
      source: sourceUrl,
      target: getDownloadTarget("inprogress6.txt"),
      tempPath: "",
      startTime: getStartTime(6),
      state: DOWNLOAD_QUEUED,
      referrer: httpUrl("referrer6"),
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "text/plain",
      preferredAction: Ci.nsIMIMEInfo.useHelperApp,
      preferredApplication: "prerredApplication6",
      autoResume: 0,

      expectedCurrentBytes: TEST_DATA_LENGTH,
      expectedTotalBytes: TEST_DATA_LENGTH,
      expectedResume: true,
      expectedContentType: "text/plain",
      expectedContent: TEST_DATA_SHORT
    },

    
    
    {
      source: sourceUrl,
      target: getDownloadTarget("inprogress7.txt"),
      tempPath: "",
      startTime: getStartTime(7),
      state: DOWNLOAD_NOTSTARTED,
      referrer: httpUrl("referrer7"),
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "text/plain",
      preferredAction: Ci.nsIMIMEInfo.useHelperApp,
      preferredApplication: "prerredApplication7",
      autoResume: 0,

      expectedCurrentBytes: TEST_DATA_LENGTH,
      expectedTotalBytes: TEST_DATA_LENGTH,
      expectedResume: true,
      expectedContentType: "text/plain",
      expectedContent: TEST_DATA_SHORT
    },

    
    
    {
      source: sourceUrl,
      target: getDownloadTarget("inprogress8.txt"),
      tempPath: yield getPartialFile("inprogress8.txt.part", true),
      startTime: getStartTime(8),
      state: DOWNLOAD_DOWNLOADING,
      referrer: httpUrl("referrer8"),
      entityID: sourceEntityId,
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "text/plain",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication8",
      autoResume: 0,

      expectedCurrentBytes: TEST_DATA_LENGTH,
      expectedTotalBytes: TEST_DATA_LENGTH,
      expectedResume: true,
      expectedContentType: "text/plain",
      expectedContent: TEST_DATA_TAINTED
    },
  ];
});





add_task(function prepareNonImportableDownloads()
{
  gDownloadsRowNonImportable = [
    
    {
      source: "",
      target: "nonimportable1.txt",
      tempPath: "",
      startTime: getStartTime(1),
      state: DOWNLOAD_PAUSED,
      referrer: "",
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "mimeType1",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication1",
      autoResume: 1
    },

    
    {
      source: httpUrl("source.txt"),
      target: "nonimportable2.txt",
      tempPath: "",
      startTime: getStartTime(2),
      state: DOWNLOAD_FAILED,
      referrer: "",
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "mimeType2",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication2",
      autoResume: 1
    },

    
    {
      source: httpUrl("source.txt"),
      target: "nonimportable3.txt",
      tempPath: "",
      startTime: getStartTime(3),
      state: DOWNLOAD_CANCELED,
      referrer: "",
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "mimeType3",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication3",
      autoResume: 1
    },

    
    {
      source: httpUrl("source.txt"),
      target: "nonimportable4.txt",
      tempPath: "",
      startTime: getStartTime(4),
      state: DOWNLOAD_BLOCKED_PARENTAL,
      referrer: "",
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "mimeType4",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication4",
      autoResume: 1
    },

    
    {
      source: httpUrl("source.txt"),
      target: "nonimportable5.txt",
      tempPath: "",
      startTime: getStartTime(5),
      state: DOWNLOAD_SCANNING,
      referrer: "",
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "mimeType5",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication5",
      autoResume: 1
    },

    
    {
      source: httpUrl("source.txt"),
      target: "nonimportable6.txt",
      tempPath: "",
      startTime: getStartTime(6),
      state: DOWNLOAD_DIRTY,
      referrer: "",
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "mimeType6",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication6",
      autoResume: 1
    },

    
    {
      source: httpUrl("source.txt"),
      target: "nonimportable7.txt",
      tempPath: "",
      startTime: getStartTime(7),
      state: DOWNLOAD_BLOCKED_POLICY,
      referrer: "",
      entityID: "",
      maxBytes: MAXBYTES_IN_DB,
      mimeType: "mimeType7",
      preferredAction: Ci.nsIMIMEInfo.saveToDisk,
      preferredApplication: "prerredApplication7",
      autoResume: 1
    },
  ];
});









add_task(function test_downloadImport()
{
  let connection = null;
  let downloadsSqlite = getTempFile("downloads.sqlite").path;

  try {
    
    connection = yield promiseEmptyDatabaseConnection({
      aPath: downloadsSqlite,
      aSchemaVersion: 9
    });

    
    
    for (let downloadRow of gDownloadsRowToImport) {
      yield promiseInsertRow(connection, downloadRow);
    }

    for (let downloadRow of gDownloadsRowNonImportable) {
      yield promiseInsertRow(connection, downloadRow);
    }

    
    do_check_eq((yield promiseTableCount(connection)),
                gDownloadsRowToImport.length +
                gDownloadsRowNonImportable.length);
  } finally {
    
    yield connection.close();
  }

  
  let list = yield promiseNewList(false);
  yield new DownloadImport(list, downloadsSqlite).import();
  let items = yield list.getAll();

  do_check_eq(items.length, gDownloadsRowToImport.length);

  for (let i = 0; i < gDownloadsRowToImport.length; i++) {
    yield checkDownload(items[i], gDownloadsRowToImport[i]);
  }
})




















function checkInvalidResult(dict, expectedErr, explanation) {
  navigator.mozDownloadManager.ondownloadstart = function() {
    ok(false, "No download should have been added!");
  };
  navigator.mozDownloadManager.adoptDownload(dict).then(
    function() {
      ok(false, "Invalid adoptDownload did not reject!");
      runTests();
    },
    function(rejectedWith) {
      is(rejectedWith, expectedErr, explanation + " rejection value");
      runTests();
    });
}



var arbitraryDate = new Date(Date.now() - 60000);

var blobContents = new Uint8Array(256);
var memBlob = new Blob([blobContents], { type: 'application/octet-stream' });
var blobStorageName;
var blobStoragePath = 'blobby.blob';

function checkAdoptedDownload(download, validPayload) {
  is(download.totalBytes, memBlob.size, 'size');
  is(download.url, validPayload.url, 'url');
  
  
  
  
  
  
  
  info('path (not checked): ' + download.path);
  is(download.storageName, validPayload.storageName, 'storageName');
  is(download.storagePath, validPayload.storagePath, 'storagePath');
  is(download.state, 'succeeded', 'state');
  is(download.contentType, validPayload.contentType, 'contentType');
  is(download.startTime.valueOf(), arbitraryDate.valueOf(), 'startTime');
  is(download.sourceAppManifestURL,
     'http://mochi.test:8888/' +
       'tests/dom/downloads/tests/testapp_downloads_adopt_download.manifest',
    'app manifest');
};

var tests = [
  function saveBlobToDeviceStorage() {
    
    
    var storage = navigator.getDeviceStorage('sdcard');
    
    
    blobStorageName = storage.storageName;
    ok(!!storage, 'have storage');
    var req = storage.addNamed(memBlob, blobStoragePath);
    req.onerror = function() {
      ok(false, 'problem saving blob to storage: ' + req.error.name);
    };
    req.onsuccess = function(evt) {
      ok(true, 'saved blob: ' + evt.target.result);
      runTests();
    };
  },
  function addValid() {
      var validPayload = {
        
        
        url: "",
        storageName: blobStorageName,
        storagePath: blobStoragePath,
        contentType: memBlob.type,
        startTime: arbitraryDate
      };
    
    
    var notifiedPromise = new Promise(function(resolve, reject) {
      navigator.mozDownloadManager.ondownloadstart = function(evt) {
        resolve(evt.download);
      };
    });

    
    navigator.mozDownloadManager.adoptDownload(validPayload).then(
      function(apiDownload) {
        checkAdoptedDownload(apiDownload, validPayload);
        ok(!!apiDownload.id, "Need a download id!");
        notifiedPromise.then(function(notifiedDownload) {
          checkAdoptedDownload(notifiedDownload, validPayload);
          is(apiDownload.id, notifiedDownload.id,
             "Notification should be for the download we adopted");
          runTests();
        });
      },
      function() {
        ok(false, "adoptDownload should not have rejected");
        runTests();
      });
  },

  function dictionaryNotProvided() {
    checkInvalidResult(undefined, "InvalidDownload");
  },
  
  function missingStorageName() {
    checkInvalidResult({
      url: "",
      
      storagePath: "relpath/filename.txt",
      contentType: "text/plain",
      startTime: arbitraryDate
    }, "InvalidDownload", "missing storage name");
  },
  function nullStorageName() {
    checkInvalidResult({
      url: "",
      storageName: null,
      storagePath: "relpath/filename.txt",
      contentType: "text/plain",
      startTime: arbitraryDate
    }, "InvalidDownload", "null storage name");
  },
  function missingStoragePath() {
    checkInvalidResult({
      url: "",
      storageName: blobStorageName,
      
      contentType: "text/plain",
      startTime: arbitraryDate
    }, "InvalidDownload", "missing storage path");
  },
  function nullStoragePath() {
    checkInvalidResult({
      url: "",
      storageName: blobStorageName,
      storagePath: null,
      contentType: "text/plain",
      startTime: arbitraryDate
    }, "InvalidDownload", "null storage path");
  },
  function missingContentType() {
    checkInvalidResult({
      url: "",
      storageName: "sdcard",
      storagePath: "relpath/filename.txt",
      
      startTime: arbitraryDate
    }, "InvalidDownload", "missing content type");
  },
  function nullContentType() {
    checkInvalidResult({
      url: "",
      storageName: "sdcard",
      storagePath: "relpath/filename.txt",
      contentType: null,
      startTime: arbitraryDate
    }, "InvalidDownload", "null content type");
  },
  
  function invalidStorageName() {
    checkInvalidResult({
      url: "",
      storageName: "ALMOST CERTAINLY DOES NOT EXIST",
      storagePath: "relpath/filename.txt",
      contentType: "text/plain",
      startTime: arbitraryDate
    }, "InvalidDownload", "invalid storage name");
  },
  
  function legitStorageInvalidPath() {
    checkInvalidResult({
      url: "",
      storageName: blobStorageName,
      storagePath: "ALMOST CERTAINLY DOES NOT EXIST",
      contentType: "text/plain",
      startTime: arbitraryDate
    }, "AdoptNoSuchFile", "invalid path");
  },
  function allDone() {
    
    
    navigator.mozDownloadManager.ondownloadstart = null;
    runTests();
  }
];

function runTests() {
  if (!tests.length) {
    finish();
    return;
  }

  var test = tests.shift();
  if (test.name) {
    info('starting test: ' + test.name);
  }
  test();
}
runTests();

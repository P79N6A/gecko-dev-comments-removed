







































const DIR_DATA = "data"
const URL_PREFIX = "http://localhost:4444/" + DIR_DATA + "/";

const PREF_APP_UPDATE_URL_OVERRIDE = "app.update.url.override";

var gUpdates;
var gUpdateCount;
var gStatus;
var gExpectedStatus;
var gCheckFunc;
var gNextRunFunc;

var gTestDir;
var gUpdater;
var gUpdatesDir;
var gUpdatesDirPath;

function run_test() {
  do_test_pending();

  var fileLocator = AUS_Cc["@mozilla.org/file/directory_service;1"]
                      .getService(AUS_Ci.nsIProperties);

  
  
  gTestDir = do_get_cwd();
  
  
  gTestDir.append("mar_test");
  if (gTestDir.exists())
    gTestDir.remove(true);
  gTestDir.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, 0755);

  
  
  var testFile = gTestDir.clone();
  testFile.append("text1");
  testFile.create(AUS_Ci.nsIFile.NORMAL_FILE_TYPE, 0644);

  var binDir = fileLocator.get("GreD", AUS_Ci.nsIFile);

  
  gUpdater = binDir.clone();
  gUpdater.append("updater.app");
  if (!gUpdater.exists()) {
    gUpdater = binDir.clone();
    gUpdater.append("updater.exe");
    if (!gUpdater.exists()) {
      gUpdater = binDir.clone();
      gUpdater.append("updater");
      if (!gUpdater.exists()) {
        do_throw("Unable to find updater binary!");
      }
    }
  }

  
  gUpdatesDir = binDir.clone();
  gUpdatesDir.append("updates");
  gUpdatesDir.append("0");

  
  gUpdatesDirPath = gUpdatesDir.path;
  if (/ /.test(gUpdatesDirPath))
    gUpdatesDirPath = '"' + gUpdatesDirPath + '"';

  startAUS();
  start_httpserver(DIR_DATA);
  do_timeout(0, "run_test_pt1()");
}

function end_test() {
  stop_httpserver();
  if (gTestDir.exists())
    gTestDir.remove(true);
  do_test_finished();
}



function run_test_helper(aUpdateXML, aMsg, aResult, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gStatus = null;
  gCheckFunc = check_test_helper_pt1;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatus = aResult;
  var url = URL_PREFIX + aUpdateXML;
  dump("Testing: " + aMsg + " - " + url + "\n");
  gPrefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_pt1() {
  do_check_eq(gUpdateCount, 1);
  gCheckFunc = check_test_helper_pt2;
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  var state = gAUS.downloadUpdate(bestUpdate, false);
  if (state == "null" || state == "failed")
    do_throw("nsIApplicationUpdateService:downloadUpdate returned " + state);
  gAUS.addDownloadListener(downloadListener);
}

function check_test_helper_pt2() {
  do_check_eq(gStatus, gExpectedStatus);
  gAUS.removeDownloadListener(downloadListener);
  gNextRunFunc();
}


function runUpdate() {
  
  
  
  gUpdater.copyTo(gUpdatesDir, gUpdater.leafName);
  var updateBin = gUpdatesDir.clone();
  updateBin.append(gUpdater.leafName);
  if (updateBin.leafName == "updater.app") {
    updateBin.append("Contents");
    updateBin.append("MacOS");
    updateBin.append("updater");
    if (!updateBin.exists())
      do_throw("Unable to find the updater executable!");
  }

  var process = AUS_Cc["@mozilla.org/process/util;1"]
                  .createInstance(AUS_Ci.nsIProcess);
  process.init(updateBin);
  var args = [gUpdatesDirPath];
  process.run(true, args, args.length);
  return process.exitValue;
}



function getTestFile(leafName) {
  var file = gTestDir.clone();
  file.append(leafName);
  if (!(file instanceof AUS_Ci.nsILocalFile))
    do_throw("File must be a nsILocalFile for this test! File: " + leafName);

  return file;
}


function getFileBytes(file) {
  var fis = AUS_Cc["@mozilla.org/network/file-input-stream;1"]
              .createInstance(AUS_Ci.nsIFileInputStream);
  fis.init(file, -1, -1, false);
  var bis = AUS_Cc["@mozilla.org/binaryinputstream;1"]
              .createInstance(AUS_Ci.nsIBinaryInputStream);
  bis.setInputStream(fis);
  var data = bis.readBytes(bis.available());
  bis.close();
  fis.close();
  return data;
}


function run_test_pt1() {
  run_test_helper("aus-0110_general-1.xml", "applying a complete mar",
                  AUS_Cr.NS_OK, run_test_pt2);
}


function run_test_pt2() {
  var exitValue = runUpdate();
  do_check_eq(exitValue, 0);

  dump("Testing: contents of files added\n");
  do_check_eq(getFileBytes(getTestFile("text1")), "ToBeModified\n");
  do_check_eq(getFileBytes(getTestFile("text2")), "ToBeDeleted\n");

  var refImage = do_get_file("data/aus-0110_general_ref_image1.png");
  var srcImage = getTestFile("image1.png");
  do_check_eq(getFileBytes(srcImage), getFileBytes(refImage));

  remove_dirs_and_files();
  run_test_pt3();
}


function run_test_pt3() {
  run_test_helper("aus-0110_general-2.xml", "applying a partial mar",
                  AUS_Cr.NS_OK, run_test_pt4);
}


function run_test_pt4() {
  var exitValue = runUpdate();
  do_check_eq(exitValue, 0);

  dump("Testing: removal of a file and contents of added / modified files\n");
  do_check_eq(getFileBytes(getTestFile("text1")), "Modified\n");
  do_check_false(getTestFile("text2").exists()); 
  do_check_eq(getFileBytes(getTestFile("text3")), "Added\n");

  var refImage = do_get_file("data/aus-0110_general_ref_image2.png");
  var srcImage = getTestFile("image1.png");
  do_check_eq(getFileBytes(srcImage), getFileBytes(refImage));

  end_test();
}


const updateCheckListener = {
  onProgress: function(request, position, totalSize) {
  },

  onCheckComplete: function(request, updates, updateCount) {
    gUpdateCount = updateCount;
    gUpdates = updates;
    dump("onCheckComplete url = " + request.channel.originalURI.spec + "\n\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  onError: function(request, update) {
    dump("onError url = " + request.channel.originalURI.spec + "\n\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(AUS_Ci.nsIUpdateCheckListener) &&
        !aIID.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};


const downloadListener = {
  onStartRequest: function(request, context) {
  },

  onProgress: function(request, context, progress, maxProgress) {
  },

  onStatus: function(request, context, status, statusText) {
  },

  onStopRequest: function(request, context, status) {
    gStatus = status;
    
    do_timeout(0, "gCheckFunc()");
  },

  QueryInterface: function(iid) {
    if (!iid.equals(AUS_Ci.nsIRequestObserver) &&
        !iid.equals(AUS_Ci.nsIProgressEventSink) &&
        !iid.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

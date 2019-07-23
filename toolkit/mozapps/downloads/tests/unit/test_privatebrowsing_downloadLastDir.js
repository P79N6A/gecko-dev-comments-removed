





































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;
const Cm = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);

const FILE_PICKER_CID = "@mozilla.org/filepicker;1";
const FILE_PICKER_ID = Components.ID(Cc[FILE_PICKER_CID].number);
const FILE_PICKER_DESCRIPTION = "File Picker Test Service";


var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);
var profileDir = null;
try {
  profileDir = dirSvc.get("ProfD", Ci.nsIFile);
} catch (e) { }
if (!profileDir) {
  
  
  var provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == "ProfD") {
        return dirSvc.get("CurProcD", Ci.nsILocalFile);
      } else if (prop == "DLoads") {
        var file = dirSvc.get("CurProcD", Ci.nsILocalFile);
        file.append("downloads.rdf");
        return file;
      }
      print("*** Throwing trying to get " + prop);
      throw Cr.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsIDirectoryProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
}

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/DownloadLastDir.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let context = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIInterfaceRequestor]),
  getInterface: XPCOMUtils.generateQI([Ci.nsIDOMWindowInternal])
};

function FilePickerService() {
}

FilePickerService.prototype = {
  _obs: Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFilePicker]),

  
  modeOpen: 0,
  modeSave: 1,
  modeGetFolder: 2,
  modeOpenMultiple: 3,
  returnOK: 0,
  returnCancel: 1,
  returnReplace: 2,
  filterAll: 1,
  filterHTML: 2,
  filterText: 4,
  filterImages: 8,
  filterXML: 16,
  filterXUL: 32,
  filterApps: 64,

  
  defaultExtension: "",
  defaultString: "",
  get displayDirectory() { return null; },
  set displayDirectory(val) {
    this._obs.notifyObservers(val, "TEST_FILEPICKER_SETDISPLAYDIRECTORY", "");
  },
  file: null,
  get files() { return null; },
  get fileURL() { return null; },
  filterIndex: 0,

  
  appendFilter: function() {},
  appendFilters: function() {},
  init: function() {
    var fileptr = Cc["@mozilla.org/supports-interface-pointer;1"].
                  createInstance(Ci.nsISupportsInterfacePointer);
    this._obs.notifyObservers(fileptr, "TEST_FILEPICKER_GETFILE", "");
    this.file = fileptr.data.QueryInterface(fileptr.dataIID);
  },
  show: function() {
    return this.returnOK;
  }
};

let factory = {
  createInstance: function(aOuter, aIid) {
    if (aOuter != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return new FilePickerService().QueryInterface(aIid);
  }
};

function run_test()
{
  let pb;
  try {
    pb = Cc["@mozilla.org/privatebrowsing;1"].
         getService(Ci.nsIPrivateBrowsingService);
  } catch (e) {
    print("PB service is not available, bail out");
    return;
  }

  
  Cm.registerFactory(FILE_PICKER_ID,
                     FILE_PICKER_DESCRIPTION,
                     FILE_PICKER_CID,
                     factory);

  let prefs = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefService).
              getBranch("browser.download.");
  let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  let launcher = Cc["@mozilla.org/helperapplauncherdialog;1"].
                 getService(Ci.nsIHelperAppLauncherDialog);
  let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties);
  let tmpDir = dirSvc.get("TmpD", Ci.nsILocalFile);
  function newDirectory() {
    let dir = tmpDir.clone();
    dir.append("testdir" + Math.floor(Math.random() * 10000));
    dir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0700);
    return dir;
  }
  function newFileInDirectory(dir) {
    let file = dir.clone();
    file.append("testfile" + Math.floor(Math.random() * 10000));
    file.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0600);
    return file;
  }
  let dir1 = newDirectory();
  let dir2 = newDirectory();
  let dir3 = newDirectory();
  let file1 = newFileInDirectory(dir1);
  let file2 = newFileInDirectory(dir2);
  let file3 = newFileInDirectory(dir3);

  let observer = {
    observe: function(aSubject, aTopic, aData) {
      switch (aTopic) {
      case "TEST_FILEPICKER_GETFILE":
        let fileptr = aSubject.QueryInterface(Ci.nsISupportsInterfacePointer);
        fileptr.data = this.file;
        fileptr.dataIID = Ci.nsILocalFile;
        break;
      case "TEST_FILEPICKER_SETDISPLAYDIRECTORY":
        this.displayDirectory = aSubject.QueryInterface(Ci.nsILocalFile);
        break;
      }
    },
    file: null,
    displayDirectory: null
  };
  obs.addObserver(observer, "TEST_FILEPICKER_GETFILE", false);
  obs.addObserver(observer, "TEST_FILEPICKER_SETDISPLAYDIRECTORY", false);

  prefs.setComplexValue("lastDir", Ci.nsILocalFile, tmpDir);

  observer.file = file1;
  let file = launcher.promptForSaveToFile(null, context, null, null, null);
  do_check_true(!!file);
  
  do_check_eq(observer.displayDirectory.path, tmpDir.path);
  
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir1.path);
  
  do_check_eq(gDownloadLastDir.file.path, dir1.path);

  pb.privateBrowsingEnabled = true;
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir1.path);
  observer.file = file2;
  observer.displayDirectory = null;
  file = launcher.promptForSaveToFile(null, context, null, null, null);
  do_check_true(!!file);
  
  do_check_eq(observer.displayDirectory.path, dir1.path);
  
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir1.path);
  
  do_check_eq(gDownloadLastDir.file.path, dir2.path);

  pb.privateBrowsingEnabled = false;
  
  do_check_eq(gDownloadLastDir.file.path, dir1.path);
  observer.file = file3;
  observer.displayDirectory = null;
  file = launcher.promptForSaveToFile(null, context, null, null, null);
  do_check_true(!!file);
  
  do_check_eq(observer.displayDirectory.path, dir1.path);
  
  do_check_eq(prefs.getComplexValue("lastDir", Ci.nsILocalFile).path, dir3.path);
  
  do_check_eq(gDownloadLastDir.file.path, dir3.path);

  
  [dir1, dir2, dir3].forEach(function(dir) dir.remove(true));
  dirSvc.QueryInterface(Ci.nsIDirectoryService).unregisterProvider(provider);
  obs.removeObserver(observer, "TEST_FILEPICKER_GETFILE", false);
  obs.removeObserver(observer, "TEST_FILEPICKER_SETDISPLAYDIRECTORY", false);
  Cm.unregisterFactory(FILE_PICKER_ID, factory);
}

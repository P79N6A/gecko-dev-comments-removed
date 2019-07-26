


const URL = "http://mochi.test:8888/migration3";
const URL2 = URL + "#2";
const URL3 = URL + "#3";
const THUMBNAIL_DIRECTORY = "thumbnails";
const PREF_STORAGE_VERSION = "browser.pagethumbnails.storage_version";

let tmp = {};
Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("resource://gre/modules/PageThumbs.jsm", tmp);
let {PageThumbsStorageMigrator} = tmp;

XPCOMUtils.defineLazyServiceGetter(this, "gDirSvc",
  "@mozilla.org/file/directory_service;1", "nsIProperties");






function runTests() {
  let dirSvc = Cc["@mozilla.org/file/directory_service;1"]
                 .getService(Ci.nsIProperties);

  
  let localProfile = FileUtils.getDir("ProfD", ["local-test"], true);
  changeLocation("ProfLD", localProfile);

  let local = FileUtils.getDir("ProfLD", [THUMBNAIL_DIRECTORY], true);
  let roaming = FileUtils.getDir("ProfD", [THUMBNAIL_DIRECTORY], true);

  
  let name = PageThumbsStorage.getLeafNameForURL(URL);
  let file = FileUtils.getFile("ProfD", [THUMBNAIL_DIRECTORY, name]);
  writeDummyFile(file);

  name = PageThumbsStorage.getLeafNameForURL(URL2);
  file = FileUtils.getFile("ProfD", [THUMBNAIL_DIRECTORY, name]);
  writeDummyFile(file);

  name = PageThumbsStorage.getLeafNameForURL(URL3);
  file = FileUtils.getFile("ProfD", [THUMBNAIL_DIRECTORY, name]);
  writeDummyFile(file);

  
  
  name = PageThumbsStorage.getLeafNameForURL(URL3);
  file = FileUtils.getFile("ProfLD", [THUMBNAIL_DIRECTORY, name]);
  writeDummyFile(file, "no-overwrite-plz");

  
  PageThumbsStorageMigrator.migrateToVersion3(localProfile.path);
  ok(true, "migration finished");

  
  yield whenFileExists(URL);
  ok(true, "first thumbnail moved");

  
  yield whenFileExists(URL2);
  ok(true, "second thumbnail moved");

  yield whenFileRemoved(roaming);
  ok(true, "roaming thumbnail directory removed");

  
  is(getFileContents(file), "no-overwrite-plz",
    "existing thumbnail was not overwritten");
}

function changeLocation(aLocation, aNewDir) {
  let oldDir = gDirSvc.get(aLocation, Ci.nsILocalFile);
  gDirSvc.undefine(aLocation);
  gDirSvc.set(aLocation, aNewDir);

  registerCleanupFunction(function () {
    gDirSvc.undefine(aLocation);
    gDirSvc.set(aLocation, oldDir);
  });
}

function writeDummyFile(aFile, aContents) {
  let fos = FileUtils.openSafeFileOutputStream(aFile);
  let data = aContents || "dummy";
  fos.write(data, data.length);
  FileUtils.closeSafeFileOutputStream(fos);
}

function getFileContents(aFile) {
  let istream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  istream.init(aFile, FileUtils.MODE_RDONLY, FileUtils.PERMS_FILE, 0);
  return NetUtil.readInputStreamToString(istream, istream.available());
}

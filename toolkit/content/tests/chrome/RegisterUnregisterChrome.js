

const NS_CHROME_MANIFESTS_FILE_LIST = "ChromeML";
const XUL_CACHE_PREF = "nglayout.debug.disable_xul_cache";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

let gDirSvc    = Cc["@mozilla.org/file/directory_service;1"].
  getService(Ci.nsIDirectoryService).QueryInterface(Ci.nsIProperties);
let gChromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].
                    getService(Ci.nsIXULChromeRegistry);
let gPrefs     = Cc["@mozilla.org/preferences-service;1"].
                    getService(Ci.nsIPrefBranch);



function copyToTemporaryFile(f)
{
  let tmpd = gDirSvc.get("ProfD", Ci.nsIFile);
  tmpf = tmpd.clone();
  tmpf.append("temp.manifest");
  tmpf.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0600);
  tmpf.remove(false);
  f.copyTo(tmpd, tmpf.leafName);
  return tmpf;
}

function dirIter(directory)
{
  var ioSvc = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
  var testsDir = ioSvc.newURI(directory, null, null)
                  .QueryInterface(Ci.nsIFileURL).file;

  let en = testsDir.directoryEntries;
  while (en.hasMoreElements()) {
    let file = en.getNext();
    yield file.QueryInterface(Ci.nsIFile);
  }
}

function getParent(path) {
  let lastSlash = path.lastIndexOf("/");
  if (lastSlash == -1) {
    lastSlash = path.lastIndexOf("\\");
    if (lastSlash == -1) {
      return "";
    }
    return '/' + path.substring(0, lastSlash).replace(/\\/g, '/');
  }
  return path.substring(0, lastSlash);
}

function copyDirToTempProfile(path, subdirname) {

  if (subdirname === undefined) {
    subdirname = "mochikit-tmp";
  }
  
  let tmpdir = gDirSvc.get("ProfD", Ci.nsIFile);
  tmpdir.append(subdirname);
  tmpdir.createUnique(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0777);

  let rootDir = getParent(path);
  if (rootDir == "") {
    return tmpdir;
  }

  
  var files = [file for (file in dirIter('file://' +rootDir))];
  for (f in files) {
    files[f].copyTo(tmpdir, "");
  }
  return tmpdir;

}

function convertChromeURI(chromeURI)
{
  let uri = Cc["@mozilla.org/network/io-service;1"].
    getService(Ci.nsIIOService).newURI(chromeURI, null, null);
  return gChromeReg.convertChromeURL(uri);
}

function chromeURIToFile(chromeURI)
{
  var jar = getJar(chromeURI);
  if (jar) {
    var tmpDir = extractJarToTmp(jar);
    let parts = chromeURI.split('/');
    if (parts[parts.length - 1] != '') {
      tmpDir.append(parts[parts.length - 1]);
    }
    return tmpDir;
  }

  return convertChromeURI(chromeURI).
    QueryInterface(Ci.nsIFileURL).file;
}  



function createManifestTemporarily(tempDir, manifestText)
{
  gPrefs.setBoolPref(XUL_CACHE_PREF, true);

  tempDir.append("temp.manifest");

  let foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                   .createInstance(Ci.nsIFileOutputStream);
  foStream.init(tempDir,
                0x02 | 0x08 | 0x20, 0664, 0); 
  foStream.write(manifestText, manifestText.length);
  foStream.close();
  let tempfile = copyToTemporaryFile(tempDir);

  Components.manager.QueryInterface(Ci.nsIComponentRegistrar).
    autoRegister(tempfile);

  gChromeReg.refreshSkins();

  return function() {
    tempfile.fileSize = 0; 
    gChromeReg.checkForNewChrome();
    gChromeReg.refreshSkins();
    gPrefs.clearUserPref(XUL_CACHE_PREF);
  }
}



function registerManifestTemporarily(manifestURI)
{
  gPrefs.setBoolPref(XUL_CACHE_PREF, true);

  let file = chromeURIToFile(manifestURI);

  let tempfile = copyToTemporaryFile(file);
  Components.manager.QueryInterface(Ci.nsIComponentRegistrar).
    autoRegister(tempfile);

  gChromeReg.refreshSkins();

  return function() {
    tempfile.fileSize = 0; 
    gChromeReg.checkForNewChrome();
    gChromeReg.refreshSkins();
    gPrefs.clearUserPref(XUL_CACHE_PREF);
  }
}

function registerManifestPermanently(manifestURI)
{
  var chromepath = chromeURIToFile(manifestURI);
  
  Components.manager.QueryInterface(Ci.nsIComponentRegistrar).
    autoRegister(chromepath);
  return chromepath;
}

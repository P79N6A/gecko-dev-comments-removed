



const Cu = Components.utils;
const {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const TargetFactory = devtools.TargetFactory;
const {console} = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
const promise = devtools.require("sdk/core/promise");
const {FileUtils} = Cu.import("resource://gre/modules/FileUtils.jsm", {});
const {NetUtil} = Cu.import("resource://gre/modules/NetUtil.jsm", {});
const ProjectEditor = devtools.require("projecteditor/projecteditor");

const TEST_URL_ROOT = "http://mochi.test:8888/browser/browser/devtools/projecteditor/test/";
const SAMPLE_WEBAPP_URL = TEST_URL_ROOT + "/helper_homepage.html";
let TEMP_PATH;
let TEMP_FOLDER_NAME = "ProjectEditor" + (new Date().getTime());


waitForExplicitFinish();





gDevTools.testing = true;
registerCleanupFunction(() => gDevTools.testing = false);


registerCleanupFunction(() => {
  
  TEMP_PATH = null;
  TEMP_FOLDER_NAME = null;
});


registerCleanupFunction(() => {
  try {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    gDevTools.closeToolbox(target);
  } catch (ex) {
    dump(ex);
  }
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});




function asyncTest(generator) {
  return () => Task.spawn(generator).then(null, ok.bind(null, false)).then(finish);
}






function addTab(url) {
  info("Adding a new tab with URL: '" + url + "'");
  let def = promise.defer();

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    info("URL '" + url + "' loading complete");
    waitForFocus(() => {
      def.resolve(tab);
    }, content);
  }, true);
  content.location = url;

  return def.promise;
}












function loadHelperScript(filePath) {
  let testDir = gTestPath.substr(0, gTestPath.lastIndexOf("/"));
  Services.scriptloader.loadSubScript(testDir + "/" + filePath, this);
}

function addProjectEditorTabForTempDirectory(opts = {}) {
  try {
    TEMP_PATH = buildTempDirectoryStructure();
  } catch (e) {
    
    
    
    info ("Project Editor temp directory creation failed.  Trying again.");
    TEMP_PATH = buildTempDirectoryStructure();
  }
  let customOpts = {
    name: "Test",
    iconUrl: "chrome://browser/skin/devtools/tool-options.svg",
    projectOverviewURL: SAMPLE_WEBAPP_URL
  };

  info ("Adding a project editor tab for editing at: " + TEMP_PATH);
  return addProjectEditorTab(opts).then((projecteditor) => {
    return projecteditor.setProjectToAppPath(TEMP_PATH, customOpts).then(() => {
      return projecteditor;
    });
  });
}

function addProjectEditorTab(opts = {}) {
  return addTab("chrome://browser/content/devtools/projecteditor-test.xul").then(() => {
    let iframe = content.document.getElementById("projecteditor-iframe");
    if (opts.menubar !== false) {
      opts.menubar = content.document.querySelector("menubar");
    }
    let projecteditor = ProjectEditor.ProjectEditor(iframe, opts);


    ok (iframe, "Tab has placeholder iframe for projecteditor");
    ok (projecteditor, "ProjectEditor has been initialized");

    return projecteditor.loaded.then((projecteditor) => {
      return projecteditor;
    });
  });
}





function buildTempDirectoryStructure() {

  let dirName = TEMP_FOLDER_NAME;
  info ("Building a temporary directory at " + dirName);

  
  let TEMP_DIR = FileUtils.getDir("TmpD", [dirName], true);
  TEMP_DIR.remove(true);

  
  TEMP_DIR = FileUtils.getDir("TmpD", [dirName], true);

  FileUtils.getDir("TmpD", [dirName, "css"], true);
  FileUtils.getDir("TmpD", [dirName, "data"], true);
  FileUtils.getDir("TmpD", [dirName, "img", "icons"], true);
  FileUtils.getDir("TmpD", [dirName, "js"], true);

  let htmlFile = FileUtils.getFile("TmpD", [dirName, "index.html"]);
  htmlFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  writeToFileSync(htmlFile, [
    '<!DOCTYPE html>',
    '<html lang="en">',
    ' <head>',
    '   <meta charset="utf-8" />',
    '   <title>ProjectEditor Temp File</title>',
    '   <link rel="stylesheet" href="style.css" />',
    ' </head>',
    ' <body id="home">',
    '   <p>ProjectEditor Temp File</p>',
    ' </body>',
    '</html>'].join("\n")
  );

  let readmeFile = FileUtils.getFile("TmpD", [dirName, "README.md"]);
  readmeFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  writeToFileSync(readmeFile, [
    '## Readme'
    ].join("\n")
  );

  let licenseFile = FileUtils.getFile("TmpD", [dirName, "LICENSE"]);
  licenseFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  writeToFileSync(licenseFile, [
   '/* This Source Code Form is subject to the terms of the Mozilla Public',
   ' * License, v. 2.0. If a copy of the MPL was not distributed with this',
   ' * file, You can obtain one at http://mozilla.org/MPL/2.0/. */'
    ].join("\n")
  );

  let cssFile = FileUtils.getFile("TmpD", [dirName, "css", "styles.css"]);
  cssFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  writeToFileSync(cssFile, [
    'body {',
    ' background: red;',
    '}'
    ].join("\n")
  );

  FileUtils.getFile("TmpD", [dirName, "js", "script.js"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);

  FileUtils.getFile("TmpD", [dirName, "img", "fake.png"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  FileUtils.getFile("TmpD", [dirName, "img", "icons", "16x16.png"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  FileUtils.getFile("TmpD", [dirName, "img", "icons", "32x32.png"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  FileUtils.getFile("TmpD", [dirName, "img", "icons", "128x128.png"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  FileUtils.getFile("TmpD", [dirName, "img", "icons", "vector.svg"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);

  return TEMP_DIR.path;
}


function writeToFile(file, data) {
  if (typeof file === "string") {
    file = new FileUtils.File(file);
  }
  info("Writing to file: " + file.path + " (exists? " + file.exists() + ")");
  let defer = promise.defer();
  var ostream = FileUtils.openSafeFileOutputStream(file);

  var converter = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  var istream = converter.convertToInputStream(data);

  
  NetUtil.asyncCopy(istream, ostream, function(status) {
    if (!Components.isSuccessCode(status)) {
      
      info("ERROR WRITING TEMP FILE", status);
    }
    defer.resolve();
  });
  return defer.promise;
}




function writeToFileSync(file, data) {
  
  var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"].
                 createInstance(Components.interfaces.nsIFileOutputStream);

  
  foStream.init(file, 0x02 | 0x08 | 0x20, 0666, 0);
  
  
  

  
  
  var converter = Components.classes["@mozilla.org/intl/converter-output-stream;1"].
                  createInstance(Components.interfaces.nsIConverterOutputStream);
  converter.init(foStream, "UTF-8", 0, 0);
  converter.writeString(data);
  converter.close(); 
}

function getTempFile(path) {
  let parts = [TEMP_FOLDER_NAME];
  parts = parts.concat(path.split("/"));
  return FileUtils.getFile("TmpD", parts);
}


function* getFileData(file) {
  if (typeof file === "string") {
    file = new FileUtils.File(file);
  }
  let def = promise.defer();

  NetUtil.asyncFetch(file, function(inputStream, status) {
    if (!Components.isSuccessCode(status)) {
      info("ERROR READING TEMP FILE", status);
    }

    
    try {
      inputStream.available();
    } catch(e) {
      def.resolve("");
      return;
    }

    var data = NetUtil.readInputStreamToString(inputStream, inputStream.available());
    def.resolve(data);
  });

  return def.promise;
}

function onceEditorCreated(projecteditor) {
  let def = promise.defer();
  projecteditor.once("onEditorCreated", (editor) => {
    def.resolve(editor);
  });
  return def.promise;
}

function onceEditorLoad(projecteditor) {
  let def = promise.defer();
  projecteditor.once("onEditorLoad", (editor) => {
    def.resolve(editor);
  });
  return def.promise;
}

function onceEditorActivated(projecteditor) {
  let def = promise.defer();
  projecteditor.once("onEditorActivated", (editor) => {
    def.resolve(editor);
  });
  return def.promise;
}

function onceEditorSave(projecteditor) {
  let def = promise.defer();
  projecteditor.once("onEditorSave", (editor, resource) => {
    def.resolve(resource);
  });
  return def.promise;
}

function onPopupShow(menu) {
  let defer = promise.defer();
  menu.addEventListener("popupshown", function onpopupshown() {
    menu.removeEventListener("popupshown", onpopupshown);
    defer.resolve();
  });
  return defer.promise;
}

function onPopupHidden(menu) {
  let defer = promise.defer();
  menu.addEventListener("popuphidden", function onpopuphidden() {
    menu.removeEventListener("popuphidden", onpopuphidden);
    defer.resolve();
  });
  return defer.promise;
}


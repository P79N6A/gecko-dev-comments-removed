const Cu = Components.utils;
const {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const {FileUtils} = Cu.import("resource://gre/modules/FileUtils.jsm", {});
const {NetUtil} = Cu.import("resource://gre/modules/NetUtil.jsm", {});
const require = devtools.require;
const promise = require("projecteditor/helpers/promise");
const ProjectEditor = require("projecteditor/projecteditor");

const SAMPLE_PATH = buildTempDirectoryStructure();
const SAMPLE_NAME = "DevTools Content";
const SAMPLE_PROJECT_URL = "http://mozilla.org";
const SAMPLE_ICON = "chrome://browser/skin/devtools/tool-options.svg";






document.addEventListener("DOMContentLoaded", function onDOMReady(e) {
  document.removeEventListener("DOMContentLoaded", onDOMReady, false);
  let iframe = document.getElementById("projecteditor-iframe");
  window.projecteditor = ProjectEditor.ProjectEditor(iframe);

  projecteditor.on("onEditorCreated", (editor) => {
    console.log("editor created: " + editor);
  });
  projecteditor.on("onEditorDestroyed", (editor) => {
    console.log("editor destroyed: " + editor);
  });
  projecteditor.on("onEditorSave", (editor, resource) => {
    console.log("editor saved: " + editor, resource.path);
  });
  projecteditor.on("onTreeSelected", (resource) => {
    console.log("tree selected: " + resource.path);
  });
  projecteditor.on("onEditorLoad", (editor) => {
    console.log("editor loaded: " + editor);
  });
  projecteditor.on("onEditorActivated", (editor) => {
    console.log("editor focused: " + editor);
  });
  projecteditor.on("onEditorDeactivated", (editor) => {
    console.log("editor blur: " + editor);
  });
  projecteditor.on("onEditorChange", (editor) => {
    console.log("editor changed: " + editor);
  });
  projecteditor.on("onEditorCursorActivity", (editor) => {
    console.log("editor cursor activity: " + editor);
  });
  projecteditor.on("onCommand", (cmd) => {
    console.log("Command: " + cmd);
  });

  projecteditor.loaded.then(() => {
    projecteditor.setProjectToAppPath(SAMPLE_PATH, {
      name: SAMPLE_NAME,
      iconUrl: SAMPLE_ICON,
      projectOverviewURL: SAMPLE_PROJECT_URL
    }).then(() => {
      let allResources = projecteditor.project.allResources();
      console.log("All resources have been loaded", allResources, allResources.map(r=>r.basename).join("|"));
    });
  });

}, false);





function buildTempDirectoryStructure() {

  
  let TEMP_DIR = FileUtils.getDir("TmpD", ["ProjectEditor"], true);
  TEMP_DIR.remove(true);

  
  TEMP_DIR = FileUtils.getDir("TmpD", ["ProjectEditor"], true);

  FileUtils.getDir("TmpD", ["ProjectEditor", "css"], true);
  FileUtils.getDir("TmpD", ["ProjectEditor", "data"], true);
  FileUtils.getDir("TmpD", ["ProjectEditor", "img", "icons"], true);
  FileUtils.getDir("TmpD", ["ProjectEditor", "js"], true);

  let htmlFile = FileUtils.getFile("TmpD", ["ProjectEditor", "index.html"]);
  htmlFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  writeToFile(htmlFile, [
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

  let readmeFile = FileUtils.getFile("TmpD", ["ProjectEditor", "README.md"]);
  readmeFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  writeToFile(readmeFile, [
    '## Readme'
    ].join("\n")
  );

  let licenseFile = FileUtils.getFile("TmpD", ["ProjectEditor", "LICENSE"]);
  licenseFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  writeToFile(licenseFile, [
   '/* This Source Code Form is subject to the terms of the Mozilla Public',
   ' * License, v. 2.0. If a copy of the MPL was not distributed with this',
   ' * file, You can obtain one at http://mozilla.org/MPL/2.0/. */'
    ].join("\n")
  );

  let cssFile = FileUtils.getFile("TmpD", ["ProjectEditor", "css", "styles.css"]);
  cssFile.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  writeToFile(cssFile, [
    'body {',
    ' background: red;',
    '}'
    ].join("\n")
  );

  FileUtils.getFile("TmpD", ["ProjectEditor", "js", "script.js"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);

  FileUtils.getFile("TmpD", ["ProjectEditor", "img", "fake.png"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  FileUtils.getFile("TmpD", ["ProjectEditor", "img", "icons", "16x16.png"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  FileUtils.getFile("TmpD", ["ProjectEditor", "img", "icons", "32x32.png"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  FileUtils.getFile("TmpD", ["ProjectEditor", "img", "icons", "128x128.png"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  FileUtils.getFile("TmpD", ["ProjectEditor", "img", "icons", "vector.svg"]).createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);

  return TEMP_DIR.path;
}



function writeToFile(file, data) {

  let defer = promise.defer();
  var ostream = FileUtils.openSafeFileOutputStream(file)

  var converter = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  var istream = converter.convertToInputStream(data);

  
  NetUtil.asyncCopy(istream, ostream, function(status) {
    if (!Components.isSuccessCode(status)) {
      
      console.log("ERROR WRITING TEMP FILE", status);
    }
  });
}







"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components

Cu.import("resource://gre/modules/FileUtils.jsm");


let gEnv = Cc["@mozilla.org/process/environment;1"]
             .getService(Ci.nsIEnvironment);
let gPythonName = gEnv.get("PYTHON");



function getExecutable(aFilename) {
  let file = FileUtils.getFile("CurProcD", [aFilename]);
  if (!file.exists()) {
    file = FileUtils.getFile("CurWorkD", []);
    while (file.path.contains("xpcshell")) {
      file = file.parent;
    }
    file.append("bin");
    file.append(aFilename);
  }
  return file;
}

let gIsWindows = Cc["@mozilla.org/xre/app-info;1"]
                 .getService(Ci.nsIXULRuntime).OS === "WINNT";
let gDmdTestFile = getExecutable("SmokeDMD" + (gIsWindows ? ".exe" : ""));

let gDmdScriptFile = getExecutable("dmd.py");

function readFile(aFile) {
  var fstream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  var cstream = Cc["@mozilla.org/intl/converter-input-stream;1"]
                  .createInstance(Ci.nsIConverterInputStream);
  fstream.init(aFile, -1, 0, 0);
  cstream.init(fstream, "UTF-8", 0, 0);

  var data = "";
  let (str = {}) {
    let read = 0;
    do {
      
      read = cstream.readString(0xffffffff, str);
      data += str.value;
    } while (read != 0);
  }
  cstream.close();                
  return data.replace(/\r/g, ""); 
}

function runProcess(aExeFile, aArgs) {
  let process = Cc["@mozilla.org/process/util;1"]
                  .createInstance(Components.interfaces.nsIProcess);
  process.init(aExeFile);
  process.run(true, aArgs, aArgs.length);
  return process.exitValue;
}

function test(aPrefix, aArgs) {
  
  
  let expectedFile = FileUtils.getFile("CurWorkD", [aPrefix + "-expected.txt"]);
  let actualFile   = FileUtils.getFile("CurWorkD", [aPrefix + "-actual.txt"]);

  

  let args = [
    gDmdScriptFile.path,
    "--filter-stacks-for-testing",
    "-o", actualFile.path
  ].concat(aArgs);

  runProcess(new FileUtils.File(gPythonName), args);

  
  
  
  

  let success;
  try {
    let rv = runProcess(new FileUtils.File("/usr/bin/diff"),
                        ["-u", expectedFile.path, actualFile.path]);
    success = rv == 0;

  } catch (e) {
    let expectedData = readFile(expectedFile);
    let actualData   = readFile(actualFile);
    success = expectedData === actualData;
    if (!success) {
      expectedData = expectedData.split("\n");
      actualData = actualData.split("\n");
      for (let i = 0; i < expectedData.length; i++) {
        print("EXPECTED:" + expectedData[i]);
      }
      for (let i = 0; i < actualData.length; i++) {
        print("  ACTUAL:" + actualData[i]);
      }
    }
  }

  ok(success, aPrefix);

  actualFile.remove(true);
}

function run_test() {
  let jsonFile, jsonFile2;

  
  
  
  
  
  

  gEnv.set("DMD", "1");
  gEnv.set(gEnv.get("DMD_PRELOAD_VAR"), gEnv.get("DMD_PRELOAD_VALUE"));

  runProcess(gDmdTestFile, []);

  let fullTestNames = ["empty", "unsampled1", "unsampled2", "sampled"];
  for (let i = 0; i < fullTestNames.length; i++) {
      let name = fullTestNames[i];
      jsonFile = FileUtils.getFile("CurWorkD", ["full-" + name + ".json"]);
      test("full-heap-" + name, ["--ignore-reports", jsonFile.path])
      test("full-reports-" + name, [jsonFile.path])
      jsonFile.remove(true);
  }

  
  
  
  

  
  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-max-frames.json"]);
  test("script-max-frames-8",
       ["--ignore-reports", "--max-frames=8", jsonFile.path]);
  test("script-max-frames-3",
       ["--ignore-reports", "--max-frames=3", "--no-fix-stacks",
        jsonFile.path]);
  test("script-max-frames-1",
       ["--ignore-reports", "--max-frames=1", jsonFile.path]);

  
  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-sort-by.json.gz"]);
  test("script-sort-by-usable",
       ["--ignore-reports", "--sort-by=usable", jsonFile.path]);
  test("script-sort-by-req",
       ["--ignore-reports", "--sort-by=req", "--no-fix-stacks", jsonFile.path]);
  test("script-sort-by-slop",
       ["--ignore-reports", "--sort-by=slop", jsonFile.path]);

  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-ignore-alloc-fns.json"]);
  test("script-ignore-alloc-fns",
       ["--ignore-reports", "--ignore-alloc-fns", jsonFile.path]);

  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-show-all-block-sizes.json"]);
  test("script-show-all-block-sizes",
       ["--ignore-reports", "--show-all-block-sizes", jsonFile.path]);

  
  
  jsonFile  = FileUtils.getFile("CurWorkD", ["script-diff1.json"]);
  jsonFile2 = FileUtils.getFile("CurWorkD", ["script-diff2.json"]);
  test("script-diff-basic",
       [jsonFile.path, jsonFile2.path]);
  test("script-diff-options",
       ["--ignore-reports", "--show-all-block-sizes",
        jsonFile.path, jsonFile2.path]);
}


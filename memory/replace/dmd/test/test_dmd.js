





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
    while (file.path.includes("xpcshell")) {
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
  let fstream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  let cstream = Cc["@mozilla.org/intl/converter-input-stream;1"]
                  .createInstance(Ci.nsIConverterInputStream);
  fstream.init(aFile, -1, 0, 0);
  cstream.init(fstream, "UTF-8", 0, 0);

  let data = "";
  let str = {};
  let read = 0;
  do {
    
    read = cstream.readString(0xffffffff, str);
    data += str.value;
  } while (read != 0);

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

  
  
  
  
  
  

  gEnv.set(gEnv.get("DMD_PRELOAD_VAR"), gEnv.get("DMD_PRELOAD_VALUE"));

  runProcess(gDmdTestFile, []);

  function test2(aTestName, aMode) {
    let name = "full-" + aTestName + "-" + aMode;
    jsonFile = FileUtils.getFile("CurWorkD", [name + ".json"]);
    test(name, [jsonFile.path]);
    jsonFile.remove(true);
  }

  

  test2("empty", "live");
  test2("empty", "dark-matter");
  test2("empty", "cumulative");

  test2("unsampled1", "live");
  test2("unsampled1", "dark-matter");

  test2("unsampled2", "dark-matter");
  test2("unsampled2", "cumulative");

  test2("sampled", "live");

  
  
  
  

  
  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-max-frames.json"]);
  test("script-max-frames-8",
       ["--max-frames=8", jsonFile.path]);
  test("script-max-frames-3",
       ["--max-frames=3", "--no-fix-stacks", jsonFile.path]);
  test("script-max-frames-1",
       ["--max-frames=1", jsonFile.path]);

  
  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-sort-by.json.gz"]);
  test("script-sort-by-usable",
       ["--sort-by=usable", jsonFile.path]);
  test("script-sort-by-req",
       ["--sort-by=req", "--no-fix-stacks", jsonFile.path]);
  test("script-sort-by-slop",
       ["--sort-by=slop", jsonFile.path]);
  test("script-sort-by-num-blocks",
       ["--sort-by=num-blocks", jsonFile.path]);

  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-ignore-alloc-fns.json"]);
  test("script-ignore-alloc-fns",
       ["--ignore-alloc-fns", jsonFile.path]);

  
  jsonFile  = FileUtils.getFile("CurWorkD", ["script-diff-live1.json"]);
  jsonFile2 = FileUtils.getFile("CurWorkD", ["script-diff-live2.json"]);
  test("script-diff-live",
       [jsonFile.path, jsonFile2.path]);

  
  jsonFile  = FileUtils.getFile("CurWorkD", ["script-diff-dark-matter1.json"]);
  jsonFile2 = FileUtils.getFile("CurWorkD", ["script-diff-dark-matter2.json"]);
  test("script-diff-dark-matter",
       [jsonFile.path, jsonFile2.path]);
}

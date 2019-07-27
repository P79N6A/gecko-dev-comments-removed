





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

function test(aJsonFile, aPrefix, aOptions) {
  
  
  let expectedFile = FileUtils.getFile("CurWorkD", [aPrefix + "-expected.txt"]);
  let actualFile   = FileUtils.getFile("CurWorkD", [aPrefix + "-actual.txt"]);

  

  let args = [
    gDmdScriptFile.path,
    "--filter-stacks-for-testing",
    "-o", actualFile.path
  ];
  args = args.concat(aOptions);
  args.push(aJsonFile.path);

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
  let jsonFile;

  
  
  
  
  
  

  gEnv.set("DMD", "1");
  gEnv.set(gEnv.get("DMD_PRELOAD_VAR"), gEnv.get("DMD_PRELOAD_VALUE"));

  runProcess(gDmdTestFile, []);

  let fullTestNames = ["empty", "unsampled1", "unsampled2", "sampled"];
  for (let i = 0; i < fullTestNames.length; i++) {
      let name = fullTestNames[i];
      jsonFile = FileUtils.getFile("CurWorkD", ["full-" + name + ".json"]);
      test(jsonFile, "full-heap-" + name, ["--ignore-reports"])
      test(jsonFile, "full-reports-" + name, [])
      jsonFile.remove(true);
  }

  
  
  
  

  
  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-max-frames.json"]);
  test(jsonFile, "script-max-frames-8", ["-r", "--max-frames=8"]);
  test(jsonFile, "script-max-frames-3", ["-r", "--max-frames=3",
                                         "--no-fix-stacks"]);
  test(jsonFile, "script-max-frames-1", ["-r", "--max-frames=1"]);

  
  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-sort-by.json.gz"]);
  test(jsonFile, "script-sort-by-usable", ["-r", "--sort-by=usable"]);
  test(jsonFile, "script-sort-by-req",    ["-r", "--sort-by=req",
                                           "--no-fix-stacks"]);
  test(jsonFile, "script-sort-by-slop",   ["-r", "--sort-by=slop"]);

  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-ignore-alloc-fns.json"]);
  test(jsonFile, "script-ignore-alloc-fns", ["-r", "--ignore-alloc-fns"]);

  
  
  jsonFile = FileUtils.getFile("CurWorkD", ["script-show-all-block-sizes.json"]);
  test(jsonFile, "script-show-all-block-sizes", ["-r", "--show-all-block-sizes"]);
}

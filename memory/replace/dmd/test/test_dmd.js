





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components

Cu.import("resource://gre/modules/FileUtils.jsm");


let gEnv = Cc["@mozilla.org/process/environment;1"]
             .getService(Ci.nsIEnvironment);
let gPythonName = gEnv.get("PYTHON");



let gDmdScriptFile = FileUtils.getFile("CurProcD", ["dmd.py"]);
if (!gDmdScriptFile.exists()) {
  gDmdScriptFile = FileUtils.getFile("CurWorkD", []);
  while (gDmdScriptFile.path.contains("xpcshell")) {
    gDmdScriptFile = gDmdScriptFile.parent;
  }
  gDmdScriptFile.append("bin");
  gDmdScriptFile.append("dmd.py");
}

function test(aJsonFile, aPrefix, aOptions) {
  
  
  let expectedFile = FileUtils.getFile("CurWorkD", [aPrefix + "-expected.txt"]);
  let actualFile   = FileUtils.getFile("CurWorkD", [aPrefix + "-actual.txt"]);

  

  let pythonFile = new FileUtils.File(gPythonName);
  let pythonProcess = Cc["@mozilla.org/process/util;1"]
                        .createInstance(Components.interfaces.nsIProcess);
  pythonProcess.init(pythonFile);

  let args = [
    gDmdScriptFile.path,
    "--filter-stacks-for-testing",
    "-o", actualFile.path
  ];
  args = args.concat(aOptions);
  args.push(aJsonFile.path);

  pythonProcess.run(true, args, args.length);

  
  

  let diffFile = new FileUtils.File("/usr/bin/diff");
  let diffProcess = Cc["@mozilla.org/process/util;1"]
                      .createInstance(Components.interfaces.nsIProcess);
  
  diffProcess.init(diffFile);

  args = ["-u", expectedFile.path, actualFile.path];
  diffProcess.run(true, args, args.length);
  let success = diffProcess.exitValue == 0;
  ok(success, aPrefix);

  actualFile.remove(true);
}

function run_test() {
  let jsonFile;

  
  
  
  
  
  
  
  
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







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

function test(aJsonFile, aKind, aOptions, aN) {
  
  
  let expectedFile =
    FileUtils.getFile("CurWorkD",
                      ["full-" + aKind + "-expected" + aN + ".txt"]);
  let actualFile =
    FileUtils.getFile("CurWorkD",
                      ["full-" + aKind + "-actual"   + aN + ".txt"]);

  

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
  ok(success, aKind + " " + aN);

  actualFile.remove(true);
}

function run_test() {
  
  
  
  
  
  
  
  
  for (let i = 1; i <= 4; i++) {
      let jsonFile = FileUtils.getFile("CurWorkD", ["full" + i + ".json"]);
      test(jsonFile, "heap", ["--ignore-reports"], i);
      test(jsonFile, "reports", [], i);
      jsonFile.remove(true);
  }
}

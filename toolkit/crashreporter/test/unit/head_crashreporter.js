

























function do_crash(setup, callback, canReturnZero)
{
  
  let ds = Components.classes["@mozilla.org/file/directory_service;1"]
    .getService(Components.interfaces.nsIProperties);
  let bin = ds.get("CurProcD", Components.interfaces.nsILocalFile);
  bin.append("xpcshell");
  if (!bin.exists()) {
    bin.leafName = "xpcshell.exe";
    do_check_true(bin.exists());
    if (!bin.exists())
      
      do_throw("Can't find xpcshell binary!");
  }
  
  let greD = ds.get("GreD", Components.interfaces.nsILocalFile);
  let headfile = do_get_file("crasher_subprocess_head.js");
  let tailfile = do_get_file("crasher_subprocess_tail.js");
  
  let process = Components.classes["@mozilla.org/process/util;1"]
                  .createInstance(Components.interfaces.nsIProcess);
  process.init(bin);
  let args = ['-g', greD.path,
              '-f', headfile.path];
  if (setup) {
    if (typeof(setup) == "function")
      
      setup = "("+setup.toSource()+")();";
    args.push('-e', setup);
  }
  args.push('-f', tailfile.path);
  try {
      process.run(true, args, args.length);
  }
  catch(ex) {} 

  if (!canReturnZero) {
    
    do_check_neq(process.exitValue, 0);
  }

  handleMinidump(callback);
}

function handleMinidump(callback)
{
  
  let minidump = null;
  let en = do_get_cwd().directoryEntries;
  while (en.hasMoreElements()) {
    let f = en.getNext().QueryInterface(Components.interfaces.nsILocalFile);
    if (f.leafName.substr(-4) == ".dmp") {
      minidump = f;
      break;
    }
  }

  if (minidump == null)
    do_throw("No minidump found!");

  let extrafile = minidump.clone();
  extrafile.leafName = extrafile.leafName.slice(0, -4) + ".extra";

  
  do_register_cleanup(function() {
          if (minidump.exists())
              minidump.remove(false);
          if (extrafile.exists())
              extrafile.remove(false);
      });
  do_check_true(extrafile.exists());
  let extra = parseKeyValuePairsFromFile(extrafile);

  if (callback)
    callback(minidump, extra);

  if (minidump.exists())
    minidump.remove(false);
  if (extrafile.exists())
    extrafile.remove(false);
}

function do_content_crash(setup, callback)
{
  do_load_child_test_harness();
  do_test_pending();

  
  
  let crashReporter =
      Components.classes["@mozilla.org/toolkit/crash-reporter;1"]
      .getService(Components.interfaces.nsICrashReporter);
  crashReporter.minidumpPath = do_get_cwd();

  let headfile = do_get_file("../unit/crasher_subprocess_head.js");
  let tailfile = do_get_file("../unit/crasher_subprocess_tail.js");
  if (setup) {
    if (typeof(setup) == "function")
      
      setup = "("+setup.toSource()+")();";
  }

  let handleCrash = function() {
    try {            
      handleMinidump(callback);
    } catch (x) {
      do_report_unexpected_exception(x);
    }
    do_test_finished();
  };
  
  sendCommand("load(\"" + headfile.path.replace(/\\/g, "/") + "\");", function()
    sendCommand(setup, function()
      sendCommand("load(\"" + tailfile.path.replace(/\\/g, "/") + "\");",
        function() do_execute_soon(handleCrash)
      )
    )
  );
}


Components.utils.import("resource://test/CrashTestUtils.jsm");
Components.utils.import("resource://gre/modules/KeyValueParser.jsm");

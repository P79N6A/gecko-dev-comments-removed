




















function do_crash(setup, callback)
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

  
  do_check_neq(process.exitValue, 0);
  
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
  do_check_true(extrafile.exists());
  let extra = parseKeyValuePairsFromFile(extrafile);

  if (callback)
    callback(minidump, extra);

  if (minidump.exists())
    minidump.remove(false);
  if (extrafile.exists())
    extrafile.remove(false);
}


function parseKeyValuePairs(text) {
  var lines = text.split('\n');
  var data = {};
  for (let i = 0; i < lines.length; i++) {
    if (lines[i] == '')
      continue;

    
    let eq = lines[i].indexOf('=');
    if (eq != -1) {
      let [key, value] = [lines[i].substring(0, eq),
                          lines[i].substring(eq + 1)];
      if (key && value)
        data[key] = value.replace("\\n", "\n", "g").replace("\\\\", "\\", "g");
    }
  }
  return data;
}

function parseKeyValuePairsFromFile(file) {
  var fstream = Components.classes["@mozilla.org/network/file-input-stream;1"].
                createInstance(Components.interfaces.nsIFileInputStream);
  fstream.init(file, -1, 0, 0);
  var is = Components.classes["@mozilla.org/intl/converter-input-stream;1"].
           createInstance(Components.interfaces.nsIConverterInputStream);
  is.init(fstream, "UTF-8", 1024, Components.interfaces.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);
  var str = {};
  var contents = '';
  while (is.readString(4096, str) != 0) {
    contents += str.value;
  }
  is.close();
  fstream.close();
  return parseKeyValuePairs(contents);
}

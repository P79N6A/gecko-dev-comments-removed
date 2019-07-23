


function run_test() {
  var nsILocalFile = Components.interfaces.nsILocalFile;
  var root = Components.classes["@mozilla.org/file/local;1"].
              createInstance(nsILocalFile);

  
  
  
  var isWindows = ("@mozilla.org/windows-registry-key;1" in Components.classes);
  if (isWindows) {
    root.initWithPath("\\\\.");
  } else {
    root.initWithPath("/");
  }
  var drives = root.directoryEntries;
  do_check_true(drives.hasMoreElements());
  while (drives.hasMoreElements()) {
    var newPath = drives.getNext().QueryInterface(nsILocalFile).path;
    do_check_eq(newPath.indexOf("\0"), -1);
  }
}

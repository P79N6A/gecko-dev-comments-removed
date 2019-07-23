const Cc = Components.classes;
const Ci = Components.interfaces;

function run_test() {
  
  var dir = do_get_profile();

  
  var pm = Cc["@mozilla.org/permissionmanager;1"].
           getService(Ci.nsIPermissionManager);

  
  var file = dir.clone();
  file.append("permissions.sqlite");
  do_check_true(file.exists());

  
  var ostream = Cc["@mozilla.org/network/file-output-stream;1"].
                createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, 0x02, 0666, 0);
  var conv = Cc["@mozilla.org/intl/converter-output-stream;1"].
             createInstance(Ci.nsIConverterOutputStream);
  conv.init(ostream, "UTF-8", 0, 0);
  for (var i = 0; i < file.fileSize; ++i)
    conv.writeString("a");
  conv.close();

  
  var hostperm = dir.clone();
  hostperm.append("hostperm.1");
  ostream.init(hostperm, 0x02 | 0x08, 0666, 0);
  ostream.close();

  
  pm.removeAll();
}

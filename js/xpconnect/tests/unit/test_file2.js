



Components.utils.importGlobalProperties(['File']);

const Ci = Components.interfaces;

function run_test() {
  

  
  var file = Components.classes["@mozilla.org/file/directory_service;1"]
             .getService(Ci.nsIProperties)
             .get("CurWorkD", Ci.nsIFile);
  file.append("xpcshell.ini");

  
  var f1 = new File(file.path);
  
  var f2 = new File(file);

  
  do_check_true(f1 instanceof File, "Should be a DOM File");
  do_check_true(f2 instanceof File, "Should be a DOM File");

  do_check_true(f1.name == "xpcshell.ini", "Should be the right file");
  do_check_true(f2.name == "xpcshell.ini", "Should be the right file");

  do_check_true(f1.type == "", "Should be the right type");
  do_check_true(f2.type == "", "Should be the right type");

  var threw = false;
  try {
    
    var f7 = File();
  } catch (e) {
    threw = true;
  }
  do_check_true(threw, "No ctor arguments should throw");

  var threw = false;
  try {
    
    var f7 = File(Date(132131532));
  } catch (e) {
    threw = true;
  }
  do_check_true(threw, "Passing a random object should fail");

  var threw = false
  try {
    
    var dir = Components.classes["@mozilla.org/file/directory_service;1"]
                        .getService(Ci.nsIProperties)
                        .get("CurWorkD", Ci.nsIFile);
    var f7 = File(dir)
  } catch (e) {
    threw = true;
  }
  do_check_true(threw, "Can't create a File object for a directory");
}

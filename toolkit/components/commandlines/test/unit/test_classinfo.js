



































function run_test() {
  var clClass = Components.classes["@mozilla.org/toolkit/command-line;1"];
  var commandLine = clClass.createInstance();
  do_check_true("length" in commandLine);
}






































function run_test() {
  const PROPERTIES = ["name", "host", "arch", "version", "pagesize",
                      "pageshift", "memmapalign", "cpucount", "memsize"];
  let sysInfo = Components.classes["@mozilla.org/system-info;1"].
                getService(Components.interfaces.nsIPropertyBag2);

  PROPERTIES.forEach(function(aPropertyName) {
    print("Testing property: " + aPropertyName);
    let value = sysInfo.getProperty(aPropertyName);
    do_check_true(!!value);
  });
}

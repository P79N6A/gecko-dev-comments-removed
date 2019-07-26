


function run_test() {
  let env = Components.classes["@mozilla.org/process/environment;1"]
                      .getService(Components.interfaces.nsIEnvironment);
  do_check_throws_nsIException(function () {
    env.QueryInterface(Components.interfaces.nsIFile);
  }, "NS_NOINTERFACE");
}


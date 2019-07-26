




Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");

let sessionCheckpointsPath;




function run_test()
{
  do_get_profile();
  sessionCheckpointsPath = OS.Path.join(OS.Constants.Path.profileDir,
                                        "sessionCheckpoints.json");
  Components.utils.import("resource://gre/modules/CrashMonitor.jsm");
  run_next_test();
}










"use strict";






add_task_in_parent_process(function* test_xpcshell_initialize_profile() {
  
  
  Cc["@mozilla.org/formautofill/startup;1"]
    .getService(Ci.nsIObserver)
    .observe(null, "profile-after-change", "");
});







































var installListenerA = {};
var installListenerB = {};
var installListenerC = {};
var installListenerD = {};
var installListenerE = {};

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "1.9");
  startupEM();

  
  do_check_eq(gEM.addInstallListener(installListenerA), 0);
  do_check_eq(gEM.addInstallListener(installListenerB), 1);
  do_check_eq(gEM.addInstallListener(installListenerC), 2);
  

  
  do_check_eq(gEM.addInstallListener(installListenerA), 0);
  do_check_eq(gEM.addInstallListener(installListenerB), 1);
  do_check_eq(gEM.addInstallListener(installListenerC), 2);
  

  
  gEM.removeInstallListenerAt(0);
  do_check_eq(gEM.addInstallListener(installListenerB), 1);
  do_check_eq(gEM.addInstallListener(installListenerC), 2);
  

  
  do_check_eq(gEM.addInstallListener(installListenerD), 3);
  

  
  gEM.removeInstallListenerAt(3);
  do_check_eq(gEM.addInstallListener(installListenerE), 3);
  

  do_check_eq(gEM.addInstallListener(installListenerD), 4);
  gEM.removeInstallListenerAt(3);
  do_check_eq(gEM.addInstallListener(installListenerE), 5);
  

  
  gEM.removeInstallListenerAt(4);
  gEM.removeInstallListenerAt(5);
  do_check_eq(gEM.addInstallListener(installListenerD), 3);
  

  
  gEM.removeInstallListenerAt(1);
  gEM.removeInstallListenerAt(2);
  gEM.removeInstallListenerAt(3);
  do_check_eq(gEM.addInstallListener(installListenerE), 0);
  gEM.removeInstallListenerAt(0);
  
}

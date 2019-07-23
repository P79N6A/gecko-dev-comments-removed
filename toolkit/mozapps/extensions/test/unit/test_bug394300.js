






































gPrefs.setBoolPref("extensions.checkUpdateSecurity", false);

do_load_httpd_js();
var server;


var updateListener = {
  _count: 0,
  
  onUpdateStarted: function onUpdateStarted()
  {
  },

  onUpdateEnded: function onUpdateEnded()
  {
    do_check_eq(this._count, 2);
    server.stop(do_test_finished);
  },

  onAddonUpdateStarted: function onAddonUpdateStarted(aAddon)
  {
  },

  onAddonUpdateEnded: function onAddonUpdateEnded(aAddon, aStatus)
  {
    this._count++;
    do_check_eq(aStatus, Components.interfaces.nsIAddonUpdateCheckListener.STATUS_UPDATE);
    do_check_eq(aAddon.version, 10);
  }
}

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");
  
  startupEM();
  
  gEM.installItemFromFile(do_get_addon("test_bug394300_1"),  NS_INSTALL_LOCATION_APPPROFILE);
  gEM.installItemFromFile(do_get_addon("test_bug394300_2"),  NS_INSTALL_LOCATION_APPPROFILE);
  
  restartEM();
  
  var updates = [
    gEM.getItemForID("bug394300_1@tests.mozilla.org"),
    gEM.getItemForID("bug394300_2@tests.mozilla.org")
  ];
  
  do_check_neq(updates[0], null);
  do_check_neq(updates[1], null);
  
  server = new nsHttpServer();
  server.registerDirectory("/", do_get_file("data"));
  server.start(4444);
  
  do_test_pending();
  
  gEM.update(updates, updates.length,
             Components.interfaces.nsIExtensionManager.UPDATE_CHECK_NEWVERSION,
             updateListener);
}

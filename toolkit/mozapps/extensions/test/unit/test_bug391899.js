





































const ADDON = "test_bug257155";
const ID = "bug257155@tests.mozilla.org";

function run_test()
{
  
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1");
  startupEM();
  
  
  gEM.installItemFromFile(do_get_addon(ADDON), NS_INSTALL_LOCATION_APPPROFILE);
  var addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.id, ID);
  restartEM();

  
  addon = gEM.getItemForID(ID);
  do_check_neq(addon, null);
  do_check_eq(addon.id, ID);

  
  addon = gEM.getItemForID("test-dummy-extension@mozilla.org");
  do_check_eq(addon, null);
  shutdownEM();
}

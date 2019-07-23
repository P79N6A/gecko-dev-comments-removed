






































function run_test()
{
  let profd = do_get_profile();
  do_check_true(profd.exists());
  do_check_true(profd.isDirectory());
  let dirSvc = Components.classes["@mozilla.org/file/directory_service;1"]
                         .getService(Components.interfaces.nsIProperties);
  let profd2 = dirSvc.get("ProfD", Components.interfaces.nsILocalFile);
  do_check_true(profd2.exists());
  do_check_true(profd2.isDirectory());
  
  do_check_true(profd.equals(profd2));
}

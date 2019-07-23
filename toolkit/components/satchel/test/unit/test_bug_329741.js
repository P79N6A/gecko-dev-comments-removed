








































function run_test()
{
  var file = do_get_file("formhistory.dat");
  var formhistFile = dirSvc.get("ProfD", Ci.nsIFile);

  
  if (formhistFile.exists())
      formhistFile.remove(false);
  do_check_false(formhistFile.exists());

  
  file.copyTo(formhistFile, "formhistory.dat");
  formhistFile.append("formhistory.dat");
  do_check_true(formhistFile.exists());

  
  var formHistory = Cc["@mozilla.org/satchel/form-history;1"].
                    getService(Ci.nsIFormHistory2);
  formHistory.removeAllEntries();
  do_check_false(formhistFile.exists());
}

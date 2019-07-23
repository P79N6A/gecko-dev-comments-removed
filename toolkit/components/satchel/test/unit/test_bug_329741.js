








































function run_test()
{
  var file = do_get_file("formhistory.dat");
  var formhistFile = dirSvc.get("ProfD", Ci.nsIFile);
  file.copyTo(formhistFile, "formhistory.dat");
  formhistFile.append("formhistory.dat");
  do_check_true(formhistFile.exists());

  var formHistory = Cc["@mozilla.org/satchel/form-history;1"].
                    getService(Ci.nsIFormHistory2);
  formHistory.removeAllEntries();
  do_check_false(formhistFile.exists());
}

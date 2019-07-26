






const Ci = Components.interfaces;

function run_test() {
  let tmpd = do_get_tempdir();
  do_check_true(tmpd.exists());
  tmpd.append("testfile");
  tmpd.create(Ci.nsIFile.NORMAL_FILE_TYPE, 600);
  do_check_true(tmpd.exists());
}

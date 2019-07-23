






































var subscriptLoaded = false;

function run_test() {
  var lf = do_get_file("file.txt");
  do_check_true(lf.exists());
  do_check_true(lf.isFile());
  
  lf = do_get_file("file.txt.notfound", true);
  do_check_false(lf.exists());
  
  lf = do_get_file("subdir/file.txt");
  do_check_true(lf.exists());
  do_check_true(lf.isFile());
  
  lf = do_get_file("subdir/");
  do_check_true(lf.exists());
  do_check_true(lf.isDirectory());
  
  lf = do_get_file("..");
  do_check_true(lf.exists());
  lf.append("unit");
  lf.append("file.txt");
  do_check_true(lf.exists());
  
  lf = do_get_cwd();
  do_check_true(lf.exists());
  do_check_true(lf.isDirectory());
}

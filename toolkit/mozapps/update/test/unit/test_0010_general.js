







































function run_test() {
  startAUS();
  
  dump("Testing: nsIApplicationUpdateService:canUpdate\n");
  do_check_true(gAUS.canUpdate);
}

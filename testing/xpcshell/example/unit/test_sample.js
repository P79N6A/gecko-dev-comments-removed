












































function run_test() {
  do_check_eq(57, 57)
  do_check_neq(1, 2)
  do_check_true(true);

  do_test_pending();
  do_timeout(100, do_test_finished);
}

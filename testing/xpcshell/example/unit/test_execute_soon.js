





var complete = false;

function run_test() {
  dump("Starting test\n");
  do_register_cleanup(function() {
    dump("Checking test completed\n");
    do_check_true(complete);
  });

  do_execute_soon(function execute_soon_callback() {
    dump("do_execute_soon callback\n");
    complete = true;
  });
}

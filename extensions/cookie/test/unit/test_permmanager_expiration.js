




let test_generator = do_run_test();

function run_test() {
  do_test_pending();
  test_generator.next();
}

function continue_test()
{
  do_run_generator(test_generator);
}

function do_run_test() {
  
  let profile = do_get_profile();

  let pm = Services.permissions;
  let permURI = NetUtil.newURI("http://example.com");
  let now = Number(Date.now());

  
  pm.add(permURI, "test/expiration-perm-exp", 1, pm.EXPIRE_TIME, now);

  
  pm.add(permURI, "test/expiration-perm-exp2", 1, pm.EXPIRE_TIME, now + 100);

  
  pm.add(permURI, "test/expiration-perm-exp3", 1, pm.EXPIRE_TIME, now + 1e6);

  
  pm.add(permURI, "test/expiration-perm-nexp", 1, pm.EXPIRE_NEVER, 0);

  
  do_check_eq(1, pm.testPermission(permURI, "test/expiration-perm-exp3"));
  do_check_eq(1, pm.testPermission(permURI, "test/expiration-perm-nexp"));

  
  do_timeout(10, continue_test);
  yield;
  do_check_eq(0, pm.testPermission(permURI, "test/expiration-perm-exp"));

  
  do_timeout(200, continue_test);
  yield;
  do_check_eq(0, pm.testPermission(permURI, "test/expiration-perm-exp2")); 

  do_finish_generator_test(test_generator);
}


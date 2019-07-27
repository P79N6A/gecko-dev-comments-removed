




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

  let pm = Services.perms;

  
  
  let permURI1 = NetUtil.newURI("http://example.com");
  let principal1 = Services.scriptSecurityManager.getNoAppCodebasePrincipal(permURI1);

  let permURI2 = NetUtil.newURI("http://example.org");
  let principal2 = Services.scriptSecurityManager.getNoAppCodebasePrincipal(permURI2);

  
  pm.addFromPrincipal(principal1, "test/remove-since", 1);

  
  do_timeout(20, continue_test);
  yield;

  let since = Number(Date.now());

  
  
  
  
  
  do_timeout(20, continue_test);
  yield;

  
  pm.addFromPrincipal(principal1, "test/remove-since-2", 1);

  
  pm.addFromPrincipal(principal2, "test/remove-since", 1);
  pm.addFromPrincipal(principal2, "test/remove-since-2", 1);

  
  pm.removeAllSince(since);

  
  do_check_eq(1, pm.testPermissionFromPrincipal(principal1, "test/remove-since"));
  
  do_check_eq(0, pm.testPermissionFromPrincipal(principal1, "test/remove-since-2"));

  
  do_check_eq(0, pm.testPermissionFromPrincipal(principal2, "test/remove-since"));
  do_check_eq(0, pm.testPermissionFromPrincipal(principal2, "test/remove-since-2"));

  do_finish_generator_test(test_generator);
}

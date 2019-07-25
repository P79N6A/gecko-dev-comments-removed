



let test_generator = do_run_test();

function run_test() {
  do_test_pending();
  test_generator.next();
}

function finish_test() {
  do_execute_soon(function() {
    test_generator.close();
    do_test_finished();
  });
}

function do_run_test()
{
  
  let profile = do_get_profile();

  
  Services.prefs.setIntPref("network.cookie.purgeAge", 1);
  Services.prefs.setIntPref("network.cookie.maxNumber", 1000);

  
  
  
  

  
  
  
  force_eviction(1101, 50);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();

  do_check_true(check_remaining_cookies(1101, 50, 1051));

  
  
  force_eviction(1101, 100);
  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_true(check_remaining_cookies(1101, 100, 1001));

  
  
  force_eviction(1101, 500);
  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_true(check_remaining_cookies(1101, 500, 1001));

  
  force_eviction(2000, 0);
  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_true(check_remaining_cookies(2000, 0, 2000));

  
  force_eviction(1100, 200);
  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_true(check_remaining_cookies(1100, 200, 1100));

  finish_test();
}



function
force_eviction(aNumberTotal, aNumberOld)
{
  Services.cookiemgr.removeAll();
  var expiry = (Date.now() + 1e6) * 1000;

  var i;
  for (i = 0; i < aNumberTotal; ++i) {
    var host = "eviction." + i + ".tests";
    Services.cookiemgr.add(host, "", "test", "eviction", false, false, false,
      expiry);

    if (i == aNumberOld - 1) {
      
      
      
      sleep(2000);
    }
  }
}






function check_remaining_cookies(aNumberTotal, aNumberOld, aNumberToExpect) {
  var enumerator = Services.cookiemgr.enumerator;

  i = 0;
  while (enumerator.hasMoreElements()) {
    var cookie = enumerator.getNext().QueryInterface(Ci.nsICookie2);
    ++i;

    if (aNumberTotal != aNumberToExpect) {
      
      var hostNumber = new Number(cookie.rawHost.split(".")[1]);
      if (hostNumber < (aNumberOld - aNumberToExpect)) break;
    }
  }

  return i == aNumberToExpect;
}


function sleep(delay)
{
  var start = Date.now();
  while (Date.now() < start + delay);
}


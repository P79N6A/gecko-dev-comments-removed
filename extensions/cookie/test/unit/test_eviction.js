


let test_generator = do_run_test();

function run_test()
{
  do_test_pending();
  do_run_generator(test_generator);
}

function continue_test()
{
  do_run_generator(test_generator);
}

function repeat_test()
{
  
  
  
  do_check_true(gPurgeAge < 64);
  gPurgeAge *= 2;
  gShortExpiry *= 2;

  do_execute_soon(function() {
    test_generator.close();
    test_generator = do_run_test();
    do_run_generator(test_generator);
  });
}


let gPurgeAge = 1;


let gShortExpiry = 2;




function get_purge_delay()
{
  return gPurgeAge * 1100 + 100;
}



function get_expiry_delay()
{
  return gShortExpiry * 1000 + 100;
}

function do_run_test()
{
  
  let profile = do_get_profile();

  
  Services.prefs.setIntPref("network.cookie.purgeAge", gPurgeAge);
  Services.prefs.setIntPref("network.cookie.maxNumber", 100);

  let expiry = Date.now() / 1000 + 1000;

  
  
  
  

  
  
  
  Services.cookiemgr.removeAll();
  if (!set_cookies(0, 5, expiry)) {
    repeat_test();
    return;
  }
  
  
  do_timeout(get_purge_delay(), continue_test);
  yield;
  if (!set_cookies(5, 111, expiry)) {
    repeat_test();
    return;
  }

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_true(check_remaining_cookies(111, 5, 106));

  
  
  Services.cookiemgr.removeAll();
  if (!set_cookies(0, 10, expiry)) {
    repeat_test();
    return;
  }
  do_timeout(get_purge_delay(), continue_test);
  yield;
  if (!set_cookies(10, 111, expiry)) {
    repeat_test();
    return;
  }

  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_true(check_remaining_cookies(111, 10, 101));

  
  
  Services.cookiemgr.removeAll();
  if (!set_cookies(0, 50, expiry)) {
    repeat_test();
    return;
  }
  do_timeout(get_purge_delay(), continue_test);
  yield;
  if (!set_cookies(50, 111, expiry)) {
    repeat_test();
    return;
  }

  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_true(check_remaining_cookies(111, 50, 101));

  
  Services.cookiemgr.removeAll();
  if (!set_cookies(0, 120, expiry)) {
    repeat_test();
    return;
  }

  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_true(check_remaining_cookies(120, 0, 120));

  
  Services.cookiemgr.removeAll();
  if (!set_cookies(0, 20, expiry)) {
    repeat_test();
    return;
  }
  do_timeout(get_purge_delay(), continue_test);
  yield;
  if (!set_cookies(20, 110, expiry)) {
    repeat_test();
    return;
  }

  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_true(check_remaining_cookies(110, 20, 110));

  
  
  Services.cookiemgr.removeAll();
  let shortExpiry = Math.floor(Date.now() / 1000) + gShortExpiry;
  if (!set_cookies(0, 20, shortExpiry)) {
    repeat_test();
    return;
  }
  do_timeout(get_expiry_delay(), continue_test);
  yield;
  if (!set_cookies(20, 110, expiry)) {
    repeat_test();
    return;
  }
  do_timeout(get_purge_delay(), continue_test);
  yield;
  if (!set_cookies(110, 111, expiry)) {
    repeat_test();
    return;
  }

  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_true(check_remaining_cookies(111, 20, 91));

  do_finish_generator_test(test_generator);
}



function set_cookies(begin, end, expiry)
{
  do_check_true(begin != end);

  let beginTime;
  for (let i = begin; i < end; ++i) {
    let host = "eviction." + i + ".tests";
    Services.cookiemgr.add(host, "", "test", "eviction", false, false, false,
      expiry);

    if (i == begin)
      beginTime = get_creationTime(i);
  }

  let endTime = get_creationTime(end - 1);
  do_check_true(begin == end - 1 || endTime > beginTime);
  if (endTime - beginTime > gPurgeAge * 1000000) {
    
    
    return false;
  }

  return true;
}

function get_creationTime(i)
{
  let host = "eviction." + i + ".tests";
  let enumerator = Services.cookiemgr.getCookiesFromHost(host);
  do_check_true(enumerator.hasMoreElements());
  let cookie = enumerator.getNext().QueryInterface(Ci.nsICookie2);
  return cookie.creationTime;
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






Components.utils.import("resource://gre/modules/Accounts.jsm");

add_task(function test_Accounts() {
  let syncExists = yield Accounts.syncAccountsExist();
  dump("Sync account exists? " + syncExists + "\n");
  let firefoxExists = yield Accounts.firefoxAccountsExist();
  dump("Firefox account exists? " + firefoxExists + "\n");
  let anyExists = yield Accounts.anySyncAccountsExist();
  dump("Any accounts exist? " + anyExists + "\n");

  
  do_check_true(!syncExists || !firefoxExists);
  do_check_eq(anyExists, firefoxExists || syncExists);

  dump("Launching setup.\n");
  Accounts.launchSetup();
});

run_next_test();

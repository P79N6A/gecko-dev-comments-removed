





Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");






function load_test_vacuum_component()
{
  const CATEGORY_NAME = "vacuum-participant";

  do_load_manifest("vacuumParticipant.manifest");

  
  
  const EXPECTED_ENTRIES = ["vacuumParticipant"];
  let catMan = Cc["@mozilla.org/categorymanager;1"].
               getService(Ci.nsICategoryManager);
  let found = false;
  let entries = catMan.enumerateCategory(CATEGORY_NAME);
  while (entries.hasMoreElements()) {
    let entry = entries.getNext().QueryInterface(Ci.nsISupportsCString).data;
    print("Check if the found category entry (" + entry + ") is expected.");
    if (EXPECTED_ENTRIES.indexOf(entry) != -1) {
      print("Check that only one test entry exists.");
      do_check_false(found);
      found = true;
    }
    else {
      
      catMan.deleteCategoryEntry("vacuum-participant", entry, false);
    }
  }
  print("Check the test entry exists.");
  do_check_true(found);
}




function synthesize_idle_daily()
{
  let vm = Cc["@mozilla.org/storage/vacuum;1"].getService(Ci.nsIObserver);
  vm.observe(null, "idle-daily", null);
}





function new_db_file(name)
{
  let file = Services.dirsvc.get("ProfD", Ci.nsIFile);
  file.append(name + ".sqlite");
  return file;
}

function run_test()
{
  do_test_pending();

  
  
  
  let conn = getDatabase(new_db_file("testVacuum"));
  conn.executeSimpleSQL("PRAGMA page_size = 1024");
  print("Check current page size.");
  let stmt = conn.createStatement("PRAGMA page_size");
  try {
    while (stmt.executeStep()) {
      do_check_eq(stmt.row.page_size, 1024);
    }
  }
  finally {
    stmt.finalize();
  }

  load_test_vacuum_component();

  run_next_test();
}

function run_next_test()
{
  if (TESTS.length == 0) {
    Services.obs.notifyObservers(null, "test-options", "dispose");
    do_test_finished();
  }
  else {
    
    Services.prefs.setIntPref("storage.vacuum.last.testVacuum.sqlite",
                              parseInt(Date.now() / 1000 - 31 * 86400));
    do_execute_soon(TESTS.shift());
  }
}

const TESTS = [

function test_common_vacuum()
{
  print("\n*** Test that a VACUUM correctly happens and all notifications are fired.");
  
  let beginVacuumReceived = false;
  Services.obs.addObserver(function (aSubject, aTopic, aData) {
    Services.obs.removeObserver(arguments.callee, aTopic, false);
    beginVacuumReceived = true;
  }, "test-begin-vacuum", false);

  
  let heavyIOTaskBeginReceived = false;
  let heavyIOTaskEndReceived = false;
  Services.obs.addObserver(function (aSubject, aTopic, aData) {
    if (heavyIOTaskBeginReceived && heavyIOTaskEndReceived) {
      Services.obs.removeObserver(arguments.callee, aTopic, false);
    }

    if (aData == "vacuum-begin") {
      heavyIOTaskBeginReceived = true;
    }
    else if (aData == "vacuum-end") {
      heavyIOTaskEndReceived = true;
    }
  }, "heavy-io-task", false);

  
  Services.obs.addObserver(function (aSubject, aTopic, aData) {
    Services.obs.removeObserver(arguments.callee, aTopic, false);
    print("Check we received onBeginVacuum");
    do_check_true(beginVacuumReceived);
    print("Check we received heavy-io-task notifications");
    do_check_true(heavyIOTaskBeginReceived);
    do_check_true(heavyIOTaskEndReceived);
    print("Received onEndVacuum");
    run_next_test();
  }, "test-end-vacuum", false);

  synthesize_idle_daily();
},

function test_skipped_if_recent_vacuum()
{
  print("\n*** Test that a VACUUM is skipped if it was run recently.");
  Services.prefs.setIntPref("storage.vacuum.last.testVacuum.sqlite",
                            parseInt(Date.now() / 1000));

  
  let vacuumObserver = {
    gotNotification: false,
    observe: function VO_observe(aSubject, aTopic, aData) {
      this.gotNotification = true;
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver])
  }
  Services.obs.addObserver(vacuumObserver, "test-begin-vacuum", false);

  
  do_timeout(2000, function () {
    print("Check VACUUM did not run.");
    do_check_false(vacuumObserver.gotNotification);
    Services.obs.removeObserver(vacuumObserver, "test-begin-vacuum");
    run_next_test();
  });

  synthesize_idle_daily();
},

function test_page_size_change()
{
  print("\n*** Test that a VACUUM changes page_size");

  
  
  print("Check that page size was updated.");
  let conn = getDatabase(new_db_file("testVacuum"));
  let stmt = conn.createStatement("PRAGMA page_size");
  try {
    while (stmt.executeStep()) {
      do_check_eq(stmt.row.page_size,  Ci.mozIStorageConnection.DEFAULT_PAGE_SIZE);
    }
  }
  finally {
    stmt.finalize();
  }

  run_next_test();
},

function test_skipped_optout_vacuum()
{
  print("\n*** Test that a VACUUM is skipped if the participant wants to opt-out.");
  Services.obs.notifyObservers(null, "test-options", "opt-out");

  
  let vacuumObserver = {
    gotNotification: false,
    observe: function VO_observe(aSubject, aTopic, aData) {
      this.gotNotification = true;
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver])
  }
  Services.obs.addObserver(vacuumObserver, "test-begin-vacuum", false);

  
  do_timeout(2000, function () {
    print("Check VACUUM did not run.");
    do_check_false(vacuumObserver.gotNotification);
    Services.obs.removeObserver(vacuumObserver, "test-begin-vacuum");
    run_next_test();
  });

  synthesize_idle_daily();
},

function test_page_size_change_with_wal()
{
  print("\n*** Test that a VACUUM changes page_size with WAL mode");
  Services.obs.notifyObservers(null, "test-options", "wal");

  
  let conn = getDatabase(new_db_file("testVacuum2"));
  conn.executeSimpleSQL("PRAGMA page_size = 1024");
  let stmt = conn.createStatement("PRAGMA page_size");
  try {
    while (stmt.executeStep()) {
      do_check_eq(stmt.row.page_size, 1024);
    }
  }
  finally {
    stmt.finalize();
  }

  
  conn.executeSimpleSQL("PRAGMA journal_mode = WAL");
  stmt = conn.createStatement("PRAGMA journal_mode");
  try {
    while (stmt.executeStep()) {
      do_check_eq(stmt.row.journal_mode, "wal");
    }
  }
  finally {
    stmt.finalize();
  }

  
  let vacuumObserver = {
    observe: function VO_observe(aSubject, aTopic, aData) {
      Services.obs.removeObserver(this, aTopic);
      print("Check page size has been updated.");
      let stmt = conn.createStatement("PRAGMA page_size");
      try {
        while (stmt.executeStep()) {
          do_check_eq(stmt.row.page_size, Ci.mozIStorageConnection.DEFAULT_PAGE_SIZE);
        }
      }
      finally {
        stmt.finalize();
      }

      print("Check journal mode has been restored.");
      stmt = conn.createStatement("PRAGMA journal_mode");
      try {
        while (stmt.executeStep()) {
          do_check_eq(stmt.row.journal_mode, "wal");
        }
      }
      finally {
        stmt.finalize();
      }

      run_next_test();
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver])
  }
  Services.obs.addObserver(vacuumObserver, "test-end-vacuum", false);

  synthesize_idle_daily();
},

function test_memory_database_crash()
{
  print("\n*** Test that we don't crash trying to vacuum a memory database");
  Services.obs.notifyObservers(null, "test-options", "memory");

  
  let vacuumObserver = {
    gotNotification: false,
    observe: function VO_observe(aSubject, aTopic, aData) {
      this.gotNotification = true;
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver])
  }
  Services.obs.addObserver(vacuumObserver, "test-begin-vacuum", false);

  
  do_timeout(2000, function () {
    print("Check VACUUM did not run.");
    do_check_false(vacuumObserver.gotNotification);
    Services.obs.removeObserver(vacuumObserver, "test-begin-vacuum");
    run_next_test();
  });

  synthesize_idle_daily();
},






















];

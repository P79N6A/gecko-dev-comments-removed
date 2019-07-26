



function run_test() {
  run_next_test();
}

add_task(function () {
  let blocklist = AM_Cc["@mozilla.org/extensions/blocklist;1"].
                  getService().wrappedJSObject;
  let scope = Components.utils.import("resource://gre/modules/osfile.jsm");

  
  blocklist._loadBlocklist();
  do_check_true(blocklist._isBlocklistLoaded());
  yield blocklist._preloadBlocklist();
  do_check_false(blocklist._isBlocklistPreloaded());
  blocklist._clear();

  
  yield blocklist._preloadBlocklist();
  do_check_false(blocklist._isBlocklistLoaded());
  do_check_true(blocklist._isBlocklistPreloaded());
  blocklist._loadBlocklist();
  do_check_true(blocklist._isBlocklistLoaded());
  do_check_false(blocklist._isBlocklistPreloaded());
  blocklist._clear();

  
  let read = scope.OS.File.read;
  scope.OS.File.read = function(...args) {
    return new Promise((resolve, reject) => {
      do_execute_soon(() => {
        blocklist._loadBlocklist();
        resolve(read(...args));
      });
    });
  }

  yield blocklist._preloadBlocklist();
  do_check_true(blocklist._isBlocklistLoaded());
  do_check_false(blocklist._isBlocklistPreloaded());
});

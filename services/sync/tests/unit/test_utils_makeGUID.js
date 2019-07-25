_("Make sure makeGUID makes guids of the right length/characters");
Cu.import("resource://services-sync/util.js");

const base64url =
  "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";

function run_test() {
  _("Create a bunch of guids to make sure they don't conflict");
  let guids = [];
  for (let i = 0; i < 1000; i++) {
    let newGuid = Utils.makeGUID();
    _("Generated " + newGuid);

    
    do_check_eq(newGuid.length, 12);
    do_check_eq(encodeURIComponent(newGuid).length, 12);

    
    do_check_true(Array.every(newGuid, function(chr) {
      return base64url.indexOf(chr) != -1;
    }));

    
    
    
    do_check_true(guids.every(function(g) { return g != newGuid; }));
    guids.push(newGuid);
  }
}

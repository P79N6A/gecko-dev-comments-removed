


Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://testing-common/services/sync/utils.js");


function run_test() {
  const PBKDF2_KEY_BYTES = 16;
  initTestLogging("Trace");
  ensureLegacyIdentityManager();

  let passphrase = "abcde-abcde-abcde-abcde";
  do_check_false(Utils.isPassphrase(passphrase));

  let normalized = Utils.normalizePassphrase(passphrase);
  _("Normalized: " + normalized);

  
  do_check_false(Utils.isPassphrase(normalized));

  
  do_check_neq(normalized, passphrase);
  do_check_eq(normalized, "abcdeabcdeabcdeabcde");

  
  Service.identity.account = "johndoe";
  Service.syncID = "1234567890";
  Service.identity.syncKey = normalized; 
  do_check_false(Utils.isPassphrase(Service.identity.syncKey));
  Service.upgradeSyncKey(Service.syncID);
  let upgraded = Service.identity.syncKey;
  _("Upgraded: " + upgraded);
  do_check_true(Utils.isPassphrase(upgraded));

  
  
  _("Sync ID: " + Service.syncID);
  let derivedKeyStr =
    Utils.derivePresentableKeyFromPassphrase(normalized,
                                             btoa(Service.syncID),
                                             PBKDF2_KEY_BYTES, true);
  _("Derived: " + derivedKeyStr);

  
  do_check_eq(derivedKeyStr, upgraded);
}




"use strict";

const kGuestKeyPref = "loop.key";
const kFxAKeyPref = "loop.key.fxa";

do_register_cleanup(function() {
  Services.prefs.clearUserPref(kGuestKeyPref);
  Services.prefs.clearUserPref(kFxAKeyPref);
  MozLoopServiceInternal.fxAOAuthTokenData = null;
  MozLoopServiceInternal.fxAOAuthProfile = null;
});

add_task(function* test_guestCreateKey() {
  
  Services.prefs.clearUserPref(kGuestKeyPref);
  MozLoopServiceInternal.fxAOAuthTokenData = null;
  MozLoopServiceInternal.fxAOAuthProfile = null;

  let key = yield MozLoopService.promiseProfileEncryptionKey();

  Assert.ok(typeof key == "string", "should generate a key");
  Assert.equal(Services.prefs.getCharPref(kGuestKeyPref), key,
    "should save the key");
});

add_task(function* test_guestGetKey() {
  
  const kFakeKey = "13572468";
  Services.prefs.setCharPref(kGuestKeyPref, kFakeKey);

  let key = yield MozLoopService.promiseProfileEncryptionKey();

  Assert.equal(key, kFakeKey, "should return existing key");
});

add_task(function* test_fxaGetKnownKey() {
  const kFakeKey = "75312468";
  
  MozLoopServiceInternal.fxAOAuthTokenData = { token_type: "bearer" };
  MozLoopServiceInternal.fxAOAuthProfile = { email: "fake@invalid.com" };
  Services.prefs.setCharPref(kFxAKeyPref, kFakeKey);

  let key = yield MozLoopService.promiseProfileEncryptionKey();

  Assert.equal(key, kFakeKey, "should return existing key");
});

add_task(function* test_fxaGetKey() {
  
  MozLoopServiceInternal.fxAOAuthTokenData = { token_type: "bearer" };
  MozLoopServiceInternal.fxAOAuthProfile = { email: "fake@invalid.com" };
  Services.prefs.clearUserPref(kFxAKeyPref);

  
  yield Assert.rejects(MozLoopService.promiseProfileEncryptionKey(),
    /No FxA key available/, "should reject as not available");
});

add_task(function test_hasEncryptionKey() {
  MozLoopServiceInternal.fxAOAuthTokenData = null;
  MozLoopServiceInternal.fxAOAuthProfile = null;

  Services.prefs.clearUserPref(kGuestKeyPref);
  Services.prefs.clearUserPref(kFxAKeyPref);

  Assert.ok(MozLoopService.hasEncryptionKey, "should return true in guest mode without a key");

  Services.prefs.setCharPref(kGuestKeyPref, "123456");

  Assert.ok(MozLoopService.hasEncryptionKey, "should return true in guest mode with a key");

  MozLoopServiceInternal.fxAOAuthTokenData = { token_type: "bearer" };
  MozLoopServiceInternal.fxAOAuthProfile = { email: "fake@invalid.com" };

  Assert.ok(!MozLoopService.hasEncryptionKey, "should return false in fxa mode without a key");

  Services.prefs.setCharPref(kFxAKeyPref, "12345678");

  Assert.ok(MozLoopService.hasEncryptionKey, "should return true in fxa mode with a key");
});

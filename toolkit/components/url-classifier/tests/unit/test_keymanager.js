



Cu.import("resource://gre/modules/osfile.jsm");

var kf = "keytest.txt"; 

function run_test() {
    run_next_test();
}

add_task(function empty_disk() {
    var jslib = Cc["@mozilla.org/url-classifier/jslib;1"]
                .getService().wrappedJSObject;
    this.PROT_UrlCryptoKeyManager = jslib.PROT_UrlCryptoKeyManager;
    yield OS.File.remove(kf);
    do_print("simulate nothing on disk, then get something from server");
    var km = new PROT_UrlCryptoKeyManager(kf, true);
    do_check_false(km.hasKey()); 
    km.maybeLoadOldKey();
    do_check_false(km.hasKey()); 
    yield km.onGetKeyResponse(null);
    do_check_false(km.hasKey()); 
    yield km.onGetKeyResponse("");
    do_check_false(km.hasKey()); 
    yield km.onGetKeyResponse("aslkaslkdf:34:a230\nskdjfaljsie");
    do_check_false(km.hasKey()); 
    var realResponse = "clientkey:24:zGbaDbx1pxoYe7siZYi8VA==\n" +
                       "wrappedkey:24:MTr1oDt6TSOFQDTvKCWz9PEn";
    yield km.onGetKeyResponse(realResponse);
    
    do_check_true(km.hasKey()); 
    do_check_eq(km.clientKey_, "zGbaDbx1pxoYe7siZYi8VA=="); 
    do_check_eq(km.wrappedKey_, "MTr1oDt6TSOFQDTvKCWz9PEn"); 

    do_print("simulate something on disk, then get something from server");
    var km = new PROT_UrlCryptoKeyManager(kf, true);
    do_check_false(km.hasKey()); 
    yield km.maybeLoadOldKey();
    do_check_true(km.hasKey()); 
    do_check_eq(km.clientKey_ , "zGbaDbx1pxoYe7siZYi8VA=="); 
    do_check_eq(km.wrappedKey_, "MTr1oDt6TSOFQDTvKCWz9PEn"); 
    var realResponse2 = "clientkey:24:dtmbEN1kgN/LmuEoYifaFw==\n" +
                        "wrappedkey:24:MTpPH3pnLDKihecOci+0W5dk";
    yield km.onGetKeyResponse(realResponse2);
    do_check_true(km.hasKey()); 
    do_check_eq(km.clientKey_, "dtmbEN1kgN/LmuEoYifaFw=="); 
    do_check_eq(km.wrappedKey_, "MTpPH3pnLDKihecOci+0W5dk"); 

    do_print("check overwriting a key on disk");
    km = new PROT_UrlCryptoKeyManager(kf, true);
    do_check_false(km.hasKey()); 
    yield km.maybeLoadOldKey();
    do_check_true(km.hasKey()); 
    do_check_eq(km.clientKey_, "dtmbEN1kgN/LmuEoYifaFw=="); 
    do_check_eq(km.wrappedKey_, "MTpPH3pnLDKihecOci+0W5dk"); 

    do_print("Test that we only fetch at most two getkey's per lifetime of the manager");
    var km = new PROT_UrlCryptoKeyManager(kf, true);
    km.reKey();
    for (var i = 0; i < km.MAX_REKEY_TRIES; i++)
      do_check_true(km.maybeReKey()); 
    do_check_false(km.maybeReKey()); 

    yield OS.File.remove(kf);
});

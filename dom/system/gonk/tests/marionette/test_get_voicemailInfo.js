


MARIONETTE_TIMEOUT = 10000;

let Cc = SpecialPowers.Cc;
let Ci = SpecialPowers.Ci;


let radioInterfaceLayer =
  Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
ok(radioInterfaceLayer);


let radioInterface = radioInterfaceLayer.getRadioInterface(0);
ok(radioInterface);


ok(radioInterface.voicemailInfo);
ok(radioInterface.voicemailInfo.number);
ok(radioInterface.voicemailInfo.displayName);

is(radioInterface.voicemailInfo.number, "+15552175049");
is(radioInterface.voicemailInfo.displayName, "Voicemail");

finish();

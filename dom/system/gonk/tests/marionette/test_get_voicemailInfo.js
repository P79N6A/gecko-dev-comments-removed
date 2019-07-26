


MARIONETTE_TIMEOUT = 10000;

let Cc = SpecialPowers.Cc;
let Ci = SpecialPowers.Ci;


let RIL = Cc["@mozilla.org/ril;1"].getService(Ci.nsIRadioInterfaceLayer);
ok(RIL);


ok(RIL.voicemailInfo);
ok(RIL.voicemailInfo.number);
ok(RIL.voicemailInfo.displayName);

is(RIL.voicemailInfo.number, "+15552175049");
is(RIL.voicemailInfo.displayName, "Voicemail");

finish();

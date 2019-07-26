


MARIONETTE_TIMEOUT = 10000;

let Cc = SpecialPowers.Cc;
let Ci = SpecialPowers.Ci;


let systemWorkerManager = Cc["@mozilla.org/telephony/system-worker-manager;1"];
ok(systemWorkerManager);


let RIL = systemWorkerManager.getService(Ci.nsIInterfaceRequestor).
          getInterface(Ci.nsIRadioInterfaceLayer);
ok(RIL);


ok(RIL.voicemailInfo);
ok(RIL.voicemailInfo.number);
ok(RIL.voicemailInfo.displayName);

is(RIL.voicemailInfo.number, "+15552175049");
is(RIL.voicemailInfo.displayName, "Voicemail");

finish();

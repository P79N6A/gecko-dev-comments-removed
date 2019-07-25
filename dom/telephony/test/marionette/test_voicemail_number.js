


MARIONETTE_TIMEOUT = 10000;

SpecialPowers.addPermission("voicemail", true, document);

let voicemail = window.navigator.mozVoicemail;
ok(voicemail instanceof MozVoicemail);


is(voicemail.number, "+15552175049");
is(voicemail.displayName, "Voicemail");

SpecialPowers.removePermission("voicemail", document);
finish();

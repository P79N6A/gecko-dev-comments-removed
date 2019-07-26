


MARIONETTE_TIMEOUT = 10000;

SpecialPowers.addPermission("voicemail", true, document);

let voicemail = window.navigator.mozVoicemail;
ok(voicemail instanceof MozVoicemail);

let serviceId = 0;


is(voicemail.getNumber(serviceId), "+15552175049");
is(voicemail.getDisplayName(serviceId), "Voicemail");

is(voicemail.getNumber(), "+15552175049");
is(voicemail.getDisplayName(), "Voicemail");

SpecialPowers.removePermission("voicemail", document);
finish();

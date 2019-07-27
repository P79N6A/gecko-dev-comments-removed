


const MARIONETTE_TIMEOUT = 60000;
const MARIONETTE_HEAD_JS = 'head.js';

startTestCommon(function() {
  let serviceId = 0;

  
  is(voicemail.getNumber(serviceId), "+15552175049");
  is(voicemail.getDisplayName(serviceId), "Voicemail");

  is(voicemail.getNumber(), "+15552175049");
  is(voicemail.getDisplayName(), "Voicemail");
});

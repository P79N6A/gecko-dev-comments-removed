


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

function test_call_mute() {
  telephony.muted = true;
  is(telephony.muted, true);
  telephony.muted = false;
  is(telephony.muted, false);
  cleanUp();
}

function cleanUp() {
  finish();
}

startTest(function() {
  test_call_mute();
});

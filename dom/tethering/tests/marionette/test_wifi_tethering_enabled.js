



MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

gTestSuite.startTetheringTest(function() {
  return gTestSuite.ensureWifiEnabled(false)
    .then(() => gTestSuite.setWifiTetheringEnabled(true));
});

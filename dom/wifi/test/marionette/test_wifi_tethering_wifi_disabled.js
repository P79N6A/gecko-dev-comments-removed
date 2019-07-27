


MARIONETTE_TIMEOUT = 60000;
MARIONETTE_HEAD_JS = 'head.js';

gTestSuite.doTestTethering(function() {
  return gTestSuite.ensureWifiEnabled(false)
    .then(() => gTestSuite.requestTetheringEnabled(true))
    .then(() => gTestSuite.requestTetheringEnabled(false))
});

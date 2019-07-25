








var phases = { "phase1": "profile1",
               "phase2": "profile1",
               "phase3": "profile1",
               "phase4": "profile1",
               "phase5": "profile1" };





Phase('phase1', [
  [Addons.install, ['unsigned-1.0.xml']],
  [Addons.verify, ['unsigned-xpi@tests.mozilla.org'], STATE_DISABLED],
  [Sync, SYNC_WIPE_SERVER],
]);

Phase('phase2', [
  [Sync],
  [Addons.verify, ['unsigned-xpi@tests.mozilla.org'], STATE_ENABLED],
  [Addons.setState, ['unsigned-xpi@tests.mozilla.org'], STATE_DISABLED],
  [Sync],
]);

Phase('phase3', [
  [Sync],
  [Addons.verify, ['unsigned-xpi@tests.mozilla.org'], STATE_DISABLED],
  [Addons.setState, ['unsigned-xpi@tests.mozilla.org'], STATE_ENABLED],
  [Sync],
]);

Phase('phase4', [
  [Sync],
  [Addons.verify, ['unsigned-xpi@tests.mozilla.org'], STATE_ENABLED],
  [Addons.uninstall, ['unsigned-xpi@tests.mozilla.org']],
  [Sync],
]);

Phase('phase5', [
  [Sync],
  [Addons.verifyNot, ['unsigned-xpi@tests.mozilla.org']],
]);

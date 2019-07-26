





EnableEngines(["addons"]);

let phases = {
  "phase01": "profile1",
  "phase02": "profile2",
  "phase03": "profile1",
  "phase04": "profile2",
  "phase05": "profile1",
  "phase06": "profile2"
};

const id = "restartless-xpi@tests.mozilla.org";


Phase("phase01", [
  [Addons.verifyNot, [id]],
  [Addons.install, [id]],
  [Addons.verify, [id], STATE_ENABLED],
  [Sync]
]);
Phase("phase02", [
  [Addons.verifyNot, [id]],
  [Sync],
  [Addons.verify, [id], STATE_ENABLED]
]);


Phase("phase03", [
  [Sync], 
  [Addons.setEnabled, [id], STATE_DISABLED],
]);
Phase("phase04", [
  [EnsureTracking],
  [Addons.uninstall, [id]],
  [Sync]
]);


Phase("phase05", [
  [Sync]
]);
Phase("phase06", [
  [Sync],
  [Addons.verifyNot, [id]]
]);

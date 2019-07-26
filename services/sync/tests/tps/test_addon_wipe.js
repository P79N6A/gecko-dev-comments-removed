







EnableEngines(["addons"]);

let phases = {
  "phase01": "profile1",
  "phase02": "profile1",
  "phase03": "profile1"
};

const id1 = "restartless-xpi@tests.mozilla.org";
const id2 = "unsigned-xpi@tests.mozilla.org";

Phase("phase01", [
  [Addons.install, [id1]],
  [Addons.install, [id2]],
  [Sync]
]);
Phase("phase02", [
  [Addons.verify, [id1], STATE_ENABLED],
  [Addons.verify, [id2], STATE_ENABLED],
  [Sync, SYNC_WIPE_CLIENT],
  [Sync]
]);
Phase("phase03", [
  [Addons.verify, [id1], STATE_ENABLED],
  [Addons.verify, [id2], STATE_ENABLED]
]);

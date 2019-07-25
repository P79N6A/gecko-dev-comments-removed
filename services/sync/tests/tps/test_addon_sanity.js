








EnableEngines(["addons"]);

let phases = { "phase1": "profile1",
               "phase2": "profile1" };

const id = "unsigned-xpi@tests.mozilla.org";

Phase("phase1", [
  [Addons.install, [id]],
  
  [Addons.verifyNot, [id]],

  
  [Sync]
]);

Phase("phase2", [
  
  [Addons.verify, [id], STATE_ENABLED]
]);










var phases = { "phase1": "profile1",
               "phase2": "profile2" };





Phase('phase1', [
  [RunMozmillTest, 'mozmill_sanity.js'],
]);

Phase('phase2', [
  [Sync],
  [RunMozmillTest, 'mozmill_sanity2.js'],
]);

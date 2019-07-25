








var phases = { "phase1": "profile1",
               "phase2": "profile2"};





var tabs1 = [
  { uri: "about:config",
    profile: "profile1"
  },
  { uri: "about:credits",
    profile: "profile1"
  },
  { uri: "data:text/html,<html><head><title>Apple</title></head><body>Apple</body></html>",
    title: "Apple",
    profile: "profile1"
  }
];

var tabs_absent = [
  { uri: "about:config",
    profile: "profile1"
  },
  { uri: "about:credits",
    profile: "profile1"
  },
];





Phase('phase1', [
  [Tabs.add, tabs1],
  [Sync]
]);

Phase('phase2', [
  [Sync],
  [Tabs.verifyNot, tabs_absent]
]);


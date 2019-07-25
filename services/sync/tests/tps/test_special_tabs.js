









EnableEngines(["tabs"]);

var phases = { "phase1": "profile1",
               "phase2": "profile2" };

var tabs1 = [
  { uri: "data:text/html,<html><head><title>Firefox</title></head><body>Firefox</body></html>",
    title: "Firefox",
    profile: "profile1"
  },
  { uri: "about:plugins",
    title: "About",
    profile: "profile1"
  },
  { uri: "about:credits",
    title: "Credits",
    profile: "profile1"
  },
  { uri: "data:text/html,<html><head><title>Mozilla</title></head><body>Mozilla</body></html>",
    title: "Mozilla",
    profile: "profile1"
  },
  { uri: "http://www.mozilla.com/en-US/firefox/sync/firstrun.html",
    title: "Firstrun",
    profile: "profile1"
  }
];

var tabs2 = [
  { uri: "data:text/html,<html><head><title>Firefox</title></head><body>Firefox</body></html>",
    title: "Firefox",
    profile: "profile1"
  },
  { uri: "data:text/html,<html><head><title>Mozilla</title></head><body>Mozilla</body></html>",
    title: "Mozilla",
    profile: "profile1"
  }
];

var tabs3 = [
  { uri: "http://www.mozilla.com/en-US/firefox/sync/firstrun.html",
    title: "Firstrun",
    profile: "profile1"
  },
  { uri: "about:plugins",
    title: "About",
    profile: "profile1"
  },
  { uri: "about:credits",
    title: "Credits",
    profile: "profile1"
  }
];




Phase('phase1', [
  [Tabs.add, tabs1],
  [Sync]
]);

Phase('phase2', [
  [Sync],
  [Tabs.verify, tabs2],
  [Tabs.verifyNot, tabs3]
]);


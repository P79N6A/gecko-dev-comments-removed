







EnableEngines(["tabs"]);

var phases = { "phase1": "profile1",
               "phase2": "profile2",
               "phase3": "profile1"};





var tabs1 = [
  { uri: "http://hg.mozilla.org/automation/crossweave/raw-file/2d9aca9585b6/pages/page1.html",
    title: "Crossweave Test Page 1",
    profile: "profile1"
  },
  { uri: "data:text/html,<html><head><title>Hello</title></head><body>Hello</body></html>",
    title: "Hello",
    profile: "profile1"
  }
];

var tabs2 = [
  { uri: "http://hg.mozilla.org/automation/crossweave/raw-file/2d9aca9585b6/pages/page3.html",
    title: "Crossweave Test Page 3",
    profile: "profile2"
  },
  { uri: "data:text/html,<html><head><title>Bye</title></head><body>Bye</body></html>",
    profile: "profile2"
  }
];





Phase('phase1', [
  [Tabs.add, tabs1],
  [Sync]
]);

Phase('phase2', [
  [Sync],
  [Tabs.verify, tabs1],
  [Tabs.add, tabs2],
  [Sync]
]);

Phase('phase3', [
  [Sync],
  [Tabs.verify, tabs2]
]);

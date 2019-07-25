








var phases = { "phase1": "profile1",
               "phase2": "profile2",
               "phase3": "profile1"};





var prefs1 = [
  { name: "browser.startup.homepage",
    value: "http://www.getfirefox.com"
  },
  { name: "browser.urlbar.maxRichResults",
    value: 20
  },
  { name: "browser.tabs.autoHide",
    value: true
  }
];

var prefs2 = [
  { name: "browser.startup.homepage",
    value: "http://www.mozilla.com"
  },
  { name: "browser.urlbar.maxRichResults",
    value: 18
  },
  { name: "browser.tabs.autoHide",
    value: false
  }
];






Phase('phase1', [
  [Prefs.modify, prefs1],
  [Prefs.verify, prefs1],
  [Sync]
]);


Phase('phase2', [
  [Sync],
  [Prefs.verify, prefs1]
]);




Phase('phase3', [
  [Prefs.modify, prefs2],
  [Prefs.verify, prefs2],
  [Sync, SYNC_WIPE_CLIENT],
  [Prefs.verify, prefs1]
]);


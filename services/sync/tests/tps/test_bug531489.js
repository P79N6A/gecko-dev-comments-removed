








var phases = { "phase1": "profile1",
               "phase2": "profile2",
               "phase3": "profile1"};






var bookmarks_initial = {
  "menu": [
    { folder: "foldera" },
    { uri: "http://www.google.com",
      title: "Google"
    }
  ],
  "menu/foldera": [
    { uri: "http://www.google.com",
      title: "Google"
    }
  ],
  "toolbar": [
    { uri: "http://www.google.com",
      title: "Google"
    }
  ]
};






Phase('phase1', [
  [Bookmarks.add, bookmarks_initial],
  [Bookmarks.verify, bookmarks_initial],
  [Sync]
]);


Phase('phase2', [
  [Sync],
  [Bookmarks.verify, bookmarks_initial]
]);



Phase('phase3', [
  [Sync],
  [Bookmarks.verify, bookmarks_initial]
]);


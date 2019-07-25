








var phases = { "phase1": "profile1",
               "phase2": "profile2"};



var bookmarks_initial = {
  "menu": [
    { folder: "testfolder",
      description: "it's just me, a test folder"
    }
  ],
  "menu/testfolder": [
    { uri: "http://www.mozilla.com",
      title: "Mozilla"
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

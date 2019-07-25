







EnableEngines(["bookmarks"]);

var phases = { "phase1": "profile1",
               "phase2": "profile2",
               "phase3": "profile1",
               "phase4": "profile2" };






var bookmarks_initial = {
  "toolbar": [
    { uri: "http://www.google.com",
      title: "Google"
    },
    { uri: "http://www.cnn.com",
      title: "CNN",
      changes: {
        position: "Google"
      }
    },
    { uri: "http://www.mozilla.com",
      title: "Mozilla"
    },
    { uri: "http://www.firefox.com",
      title: "Firefox",
      changes: {
        position: "Mozilla"
      }
    }
  ]
};

var bookmarks_after_move = {
  "toolbar": [
    { uri: "http://www.cnn.com",
      title: "CNN"
    },
    { uri: "http://www.google.com",
      title: "Google"
    },
    { uri: "http://www.firefox.com",
      title: "Firefox"
    },
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


Phase('phase3', [
  [Sync],
  [Bookmarks.verify, bookmarks_initial],
  [Bookmarks.modify, bookmarks_initial],
  [Bookmarks.verify, bookmarks_after_move],
  [Sync],
]);



Phase('phase4', [
  [Sync],
  [Bookmarks.verify, bookmarks_after_move]
]);










EnableEngines(["bookmarks"]);

var phases = { "phase1": "profile1",
               "phase2": "profile2",
               "phase3": "profile1",
               "phase4": "profile2" };






var bookmarks_initial = {
  "menu": [
    { uri: "http://www.google.com",
      loadInSidebar: true,
      tags: [ "google", "computers", "internet", "www"]
    },
    { uri: "http://bugzilla.mozilla.org/show_bug.cgi?id=%s",
      title: "Bugzilla",
      keyword: "bz"
    },
    { folder: "foldera" },
    { uri: "http://www.mozilla.com" },
    { separator: true },
    { folder: "folderb" }
  ],
  "menu/foldera": [
    { uri: "http://www.yahoo.com",
      title: "testing Yahoo"
    },
    { uri: "http://www.cnn.com",
      description: "This is a description of the site a at www.cnn.com"
    },
    { livemark: "Livemark1",
      feedUri: "http://rss.wunderground.com/blog/JeffMasters/rss.xml",
      siteUri: "http://www.wunderground.com/blog/JeffMasters/show.html"
    }
  ],
  "menu/folderb": [
    { uri: "http://www.apple.com",
      tags: [ "apple", "mac" ]
    }
  ],
  "toolbar": [
    { uri: "place:queryType=0&sort=8&maxResults=10&beginTimeRef=1&beginTime=0",
      title: "Visited Today"
    }
  ]
};


var bookmarks_to_delete = {
  "menu": [
    { uri: "http://www.google.com",
      loadInSidebar: true,
      tags: [ "google", "computers", "internet", "www"]
    }
  ],
  "menu/foldera": [
    { uri: "http://www.yahoo.com",
      title: "testing Yahoo"
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
  [Bookmarks.delete, bookmarks_to_delete],
  [Bookmarks.verifyNot, bookmarks_to_delete],
  [Sync, SYNC_WIPE_CLIENT],
  [Bookmarks.verify, bookmarks_initial]
]);


Phase('phase4', [
  [Sync],
  [Bookmarks.verify, bookmarks_initial]
]);

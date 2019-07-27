





let tests = [
  {
    desc: "nsNavHistoryFolderResultNode: Basic test, asynchronously open and " +
          "close container with a single child",

    loading: function (node, newState, oldState) {
      this.checkStateChanged("loading", 1);
      this.checkArgs("loading", node, oldState, node.STATE_CLOSED);
    },

    opened: function (node, newState, oldState) {
      this.checkStateChanged("opened", 1);
      this.checkState("loading", 1);
      this.checkArgs("opened", node, oldState, node.STATE_LOADING);

      print("Checking node children");
      compareArrayToResult(this.data, node);

      print("Closing container");
      node.containerOpen = false;
    },

    closed: function (node, newState, oldState) {
      this.checkStateChanged("closed", 1);
      this.checkState("opened", 1);
      this.checkArgs("closed", node, oldState, node.STATE_OPENED);
      this.success();
    }
  },

  {
    desc: "nsNavHistoryFolderResultNode: After async open and no changes, " +
          "second open should be synchronous",

    loading: function (node, newState, oldState) {
      this.checkStateChanged("loading", 1);
      this.checkState("closed", 0);
      this.checkArgs("loading", node, oldState, node.STATE_CLOSED);
    },

    opened: function (node, newState, oldState) {
      let cnt = this.checkStateChanged("opened", 1, 2);
      let expectOldState = cnt === 1 ? node.STATE_LOADING : node.STATE_CLOSED;
      this.checkArgs("opened", node, oldState, expectOldState);

      print("Checking node children");
      compareArrayToResult(this.data, node);

      print("Closing container");
      node.containerOpen = false;
    },

    closed: function (node, newState, oldState) {
      let cnt = this.checkStateChanged("closed", 1, 2);
      this.checkArgs("closed", node, oldState, node.STATE_OPENED);

      switch (cnt) {
      case 1:
        node.containerOpen = true;
        break;
      case 2:
        this.success();
        break;
      }
    }
  },

  {
    desc: "nsNavHistoryFolderResultNode: After closing container in " +
          "loading(), opened() should not be called",

    loading: function (node, newState, oldState) {
      this.checkStateChanged("loading", 1);
      this.checkArgs("loading", node, oldState, node.STATE_CLOSED);
      print("Closing container");
      node.containerOpen = false;
    },

    opened: function (node, newState, oldState) {
      do_throw("opened should not be called");
    },

    closed: function (node, newState, oldState) {
      this.checkStateChanged("closed", 1);
      this.checkState("loading", 1);
      this.checkArgs("closed", node, oldState, node.STATE_LOADING);
      this.success();
    }
  }
];







function Test() {
  
  this.stateCounts = {};
  
  this.deferNextTest = Promise.defer();
}

Test.prototype = {
  












  checkArgs: function (aNewState, aNode, aOldState, aExpectOldState) {
    print("Node passed on " + aNewState + " should be result.root");
    do_check_eq(this.result.root, aNode);
    print("Old state passed on " + aNewState + " should be " + aExpectOldState);

    
    
    
    do_check_true(aOldState === aExpectOldState);
  },

  







  checkStateChanged: function (aState, aExpectedMin, aExpectedMax) {
    print(aState + " state change observed");
    if (!this.stateCounts.hasOwnProperty(aState))
      this.stateCounts[aState] = 0;
    this.stateCounts[aState]++;
    return this.checkState(aState, aExpectedMin, aExpectedMax);
  },

  













  checkState: function (aState, aExpectedMin, aExpectedMax) {
    let cnt = this.stateCounts[aState] || 0;
    if (aExpectedMax === undefined)
      aExpectedMax = aExpectedMin;
    if (aExpectedMin === aExpectedMax) {
      print(aState + " should be observed only " + aExpectedMin +
            " times (actual = " + cnt + ")");
    }
    else {
      print(aState + " should be observed at least " + aExpectedMin +
            " times and at most " + aExpectedMax + " times (actual = " +
            cnt + ")");
    }
    do_check_true(cnt >= aExpectedMin && cnt <= aExpectedMax);
    return cnt;
  },

  


  openContainer: function () {
    
    
    let self = this;
    this.observer = {
      containerStateChanged: function (container, oldState, newState) {
        print("New state passed to containerStateChanged() should equal the " +
              "container's current state");
        do_check_eq(newState, container.state);

        try {
          switch (newState) {
          case Ci.nsINavHistoryContainerResultNode.STATE_LOADING:
            self.loading(container, newState, oldState);
            break;
          case Ci.nsINavHistoryContainerResultNode.STATE_OPENED:
            self.opened(container, newState, oldState);
            break;
          case Ci.nsINavHistoryContainerResultNode.STATE_CLOSED:
            self.closed(container, newState, oldState);
            break;
          default:
            do_throw("Unexpected new state! " + newState);
          }
        }
        catch (err) {
          do_throw(err);
        }
      },
    };
    this.result.addObserver(this.observer, false);

    print("Opening container");
    this.result.root.containerOpen = true;
  },

  


  run: function () {
    this.openContainer();
    return this.deferNextTest.promise;
  },

  



  setup: function*() {
    
    this.data = DataHelper.makeDataArray([
      { type: "bookmark" },
      { type: "separator" },
      { type: "folder" },
      { type: "bookmark", uri: "place:terms=foo" }
    ]);
    yield task_populateDB(this.data);

    
    this.query = PlacesUtils.history.getNewQuery();
    this.query.setFolders([DataHelper.defaults.bookmark.parent], 1);
    this.opts = PlacesUtils.history.getNewQueryOptions();
    this.opts.asyncEnabled = true;
    this.result = PlacesUtils.history.executeQuery(this.query, this.opts);
  },

  



  success: function () {
    this.result.removeObserver(this.observer);

    
    this.deferNextTest.resolve();
  }
};




let DataHelper = {
  defaults: {
    bookmark: {
      parent: PlacesUtils.bookmarks.unfiledBookmarksFolder,
      uri: "http://example.com/",
      title: "test bookmark"
    },

    folder: {
      parent: PlacesUtils.bookmarks.unfiledBookmarksFolder,
      title: "test folder"
    },

    separator: {
      parent: PlacesUtils.bookmarks.unfiledBookmarksFolder
    }
  },

  







  makeDataArray: function DH_makeDataArray(aData) {
    let self = this;
    return aData.map(function (dat) {
      let type = dat.type;
      dat = self._makeDataWithDefaults(dat, self.defaults[type]);
      switch (type) {
      case "bookmark":
        return {
          isBookmark: true,
          uri: dat.uri,
          parentFolder: dat.parent,
          index: PlacesUtils.bookmarks.DEFAULT_INDEX,
          title: dat.title,
          isInQuery: true
        };
      case "separator":
        return {
          isSeparator: true,
          parentFolder: dat.parent,
          index: PlacesUtils.bookmarks.DEFAULT_INDEX,
          isInQuery: true
        };
      case "folder":
        return {
          isFolder: true,
          parentFolder: dat.parent,
          index: PlacesUtils.bookmarks.DEFAULT_INDEX,
          title: dat.title,
          isInQuery: true
        };
      default:
        do_throw("Unknown data type when populating DB: " + type);
      }
    });
  },

  









  _makeDataWithDefaults: function DH__makeDataWithDefaults(aData, aDefaults) {
    let dat = {};
    for (let [prop, val] in Iterator(aDefaults)) {
      dat[prop] = aData.hasOwnProperty(prop) ? aData[prop] : val;
    }
    return dat;
  }
};

function run_test()
{
  run_next_test();
}

add_task(function* test_async()
{
  for (let [, test] in Iterator(tests)) {
    yield PlacesUtils.bookmarks.eraseEverything();

    test.__proto__ = new Test();
    yield test.setup();

    print("------ Running test: " + test.desc);
    yield test.run();
  }

  yield PlacesUtils.bookmarks.eraseEverything();
  print("All tests done, exiting");
});

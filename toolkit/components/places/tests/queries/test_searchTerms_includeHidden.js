







let timeInMicroseconds = Date.now() * 1000;

const VISITS = [
  { isVisit: true,
    transType: TRANSITION_TYPED,
    uri: "http://redirect.example.com/",
    title: "example",
    isRedirect: true,
    lastVisit: timeInMicroseconds--
  },
  { isVisit: true,
    transType: TRANSITION_TYPED,
    uri: "http://target.example.com/",
    title: "example",
    lastVisit: timeInMicroseconds--
  }
];

const TEST_DATA = [
  { searchTerms: "example",
    includeHidden: true,
    expectedResults: 2
  },
  { searchTerms: "example",
    includeHidden: false,
    expectedResults: 1
  },
  { searchTerms: "redir",
    includeHidden: true,
    expectedResults: 1
  }
];

function run_test() {
  populateDB(VISITS);

  for (let data of TEST_DATA) {
    let query = PlacesUtils.history.getNewQuery();
    query.searchTerms = data.searchTerms;
    let options = PlacesUtils.history.getNewQueryOptions();
    options.includeHidden = data.includeHidden;

    let root = PlacesUtils.history.executeQuery(query, options).root;
    root.containerOpen = true;
    let cc = root.childCount;
    root.containerOpen = false;
    do_check_eq(cc, data.expectedResults);
  }
}

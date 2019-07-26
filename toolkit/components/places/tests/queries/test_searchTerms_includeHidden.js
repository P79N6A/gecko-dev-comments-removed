







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

const HIDDEN_VISITS = [
  { isVisit: true,
    transType: TRANSITION_FRAMED_LINK,
    uri: "http://hidden.example.com/",
    title: "red",
    lastVisit: timeInMicroseconds--
  },
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
  { searchTerms: "red",
    includeHidden: true,
    expectedResults: 1
  }
];

function run_test()
{
  run_next_test();
}

add_task(function test_initalize()
{
  yield task_populateDB(VISITS);
});

add_task(function test_searchTerms_includeHidden()
{
  for (let data of TEST_DATA) {
    let query = PlacesUtils.history.getNewQuery();
    query.searchTerms = data.searchTerms;
    let options = PlacesUtils.history.getNewQueryOptions();
    options.includeHidden = data.includeHidden;

    let root = PlacesUtils.history.executeQuery(query, options).root;
    root.containerOpen = true;

    let cc = root.childCount;
    
    yield task_populateDB(HIDDEN_VISITS);
    let cc_update = root.childCount;

    root.containerOpen = false;

    do_check_eq(cc, data.expectedResults);
    do_check_eq(cc_update, data.expectedResults + (data.includeHidden ? 1 : 0));

    PlacesUtils.bhistory.removePage(uri("http://hidden.example.com/"));
  }
});

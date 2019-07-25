















let testData = [
  {
    isVisit: true,
    uri: "file:///directory/1",
    lastVisit: today,
    isInQuery: true
  },
  {
    isVisit: true,
    uri: "http://example.com/1",
    lastVisit: today,
    isInQuery: true
  },
  {
    isVisit: true,
    uri: "http://example.com/2",
    lastVisit: today,
    isInQuery: true
  },
  {
    isVisit: true,
    uri: "file:///directory/2",
    lastVisit: olderthansixmonths,
    isInQuery: true
  },
  {
    isVisit: true,
    uri: "http://example.com/3",
    lastVisit: olderthansixmonths,
    isInQuery: true
  },
  {
    isVisit: true,
    uri: "http://example.com/4",
    lastVisit: olderthansixmonths,
    isInQuery: true
  },
  {
    isVisit: true,
    uri: "http://example.net/1",
    lastVisit: olderthansixmonths + 1,
    isInQuery: true
  }
];
let domainsInRange = [2, 3];
let leveledTestData = [
                       [[0],    
                        [1,2]], 
                       
                       [[3],    
                        [4,5],  
                        [6]     
                        ]];



let testDataAddedLater = [
  {
    isVisit: true,
    uri: "http://example.com/5",
    lastVisit: olderthansixmonths,
    isInQuery: true,
    levels: [1,1]
  },
  {
    isVisit: true,
    uri: "http://example.com/6",
    lastVisit: olderthansixmonths,
    isInQuery: true,
    levels: [1,1]
  },
  {
    isVisit: true,
    uri: "http://example.com/7",
    lastVisit: today,
    isInQuery: true,
    levels: [0,1]
  },
  {
    isVisit: true,
    uri: "file:///directory/3",
    lastVisit: today,
    isInQuery: true,
    levels: [0,0]
  }
];
function run_test() {
  populateDB(testData);

  
  
  
  let isLinux = ("@mozilla.org/gnome-gconf-service;1" in Components.classes);
  if (isLinux)
    return;

  
  
  
  
  
  
  
  let roots = [];

  let query = PlacesUtils.history.getNewQuery();
  let options = PlacesUtils.history.getNewQueryOptions();
  options.resultType = Ci.nsINavHistoryQueryOptions.RESULTS_AS_DATE_SITE_QUERY;

  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;

  
  do_check_eq(root.childCount, leveledTestData.length);

  
  for (let index = 0; index < leveledTestData.length; index++) {
    let node = root.getChild(index);
    checkFirstLevel(index, node, roots);
  }

  
  testDataAddedLater.forEach(function(visit) {
    populateDB([visit]);
    let oldLength = testData.length;
    let i = visit.levels[0];
    let j = visit.levels[1];
    testData.push(visit);
    leveledTestData[i][j].push(oldLength);
    compareArrayToResult(leveledTestData[i][j].
                         map(function(x) testData[x]), roots[i][j]);
  });

  for (let i = 0; i < roots.length; i++) {
    for (let j = 0; j < roots[i].length; j++)
      roots[i][j].containerOpen = false;
  }

  root.containerOpen = false;
}

function checkFirstLevel(index, node, roots) {
    node.containerOpen = true;

    do_check_true(PlacesUtils.nodeIsDay(node));
    PlacesUtils.asQuery(node);
    let queries = node.getQueries();
    let options = node.queryOptions;

    do_check_eq(queries.length, 1);
    let query = queries[0];

    do_check_true(query.hasBeginTime && query.hasEndTime);

    
    let root = PlacesUtils.history.executeQuery(query, options).root;
    roots.push([]);
    root.containerOpen = true;

    do_check_eq(root.childCount, leveledTestData[index].length);
    for (var secondIndex = 0; secondIndex < root.childCount; secondIndex++) {
      let child = PlacesUtils.asQuery(root.getChild(secondIndex));
      checkSecondLevel(index, secondIndex, child, roots);
    }
    root.containerOpen = false;
    node.containerOpen = false;
}

function checkSecondLevel(index, secondIndex, child, roots) {
    let queries = child.getQueries();
    let options = child.queryOptions;

    do_check_eq(queries.length, 1);
    let query = queries[0];

    do_check_true(query.hasDomain);
    do_check_true(query.hasBeginTime && query.hasEndTime);

    let root = PlacesUtils.history.executeQuery(query, options).root;
    
    
    roots[index].push(root);

    
    
    root.containerOpen = true;
    compareArrayToResult(leveledTestData[index][secondIndex].
                         map(function(x) testData[x]), root);
    
    
}


function newPset() {
  let pset = Cc["@mozilla.org/url-classifier/prefixset;1"]
            .createInstance(Ci.nsIUrlClassifierPrefixSet);
  pset.init("all");
  return pset;
}



function arrContains(arr, target) {
  let start = 0;
  let end = arr.length - 1;
  let i = 0;

  while (end > start) {
    i = start + (end - start >> 1);
    let value = arr[i];

    if (value < target)
      start = i+1;
    else if (value > target)
      end = i-1;
    else
      break;
  }
  if (start == end)
    i = start;

  return (!(i < 0 || i >= arr.length) && arr[i] == target);
}



function checkContents(pset, prefixes) {
  var outcount = {}, outset = {};
  outset = pset.getPrefixes(outcount);
  let inset = prefixes;
  do_check_eq(inset.length, outset.length);
  inset.sort(function(x,y) x - y);
  for (let i = 0; i < inset.length; i++) {
    do_check_eq(inset[i], outset[i]);
  }
}

function wrappedProbe(pset, prefix) {
  let dummy = {};
  return pset.probe(prefix, dummy);
};






function doRandomLookups(pset, prefixes, N) {
  for (let i = 0; i < N; i++) {
    let randInt = prefixes[0];
    while (arrContains(prefixes, randInt))
      randInt = Math.floor(Math.random() * Math.pow(2, 32));

    do_check_false(wrappedProbe(pset, randInt));
  }
}




function doExpectedLookups(pset, prefixes, N) {
  for (let i = 0; i < N; i++) {
    prefixes.forEach(function (x) {
      dump("Checking " + x + "\n");
      do_check_true(wrappedProbe(pset, x));
    });
  }
}



function testBasicPset() {
  let pset = Cc["@mozilla.org/url-classifier/prefixset;1"]
               .createInstance(Ci.nsIUrlClassifierPrefixSet);
  let prefixes = [2,50,100,2000,78000,1593203];
  pset.setPrefixes(prefixes, prefixes.length);

  do_check_true(wrappedProbe(pset, 100));
  do_check_false(wrappedProbe(pset, 100000));
  do_check_true(wrappedProbe(pset, 1593203));
  do_check_false(wrappedProbe(pset, 999));
  do_check_false(wrappedProbe(pset, 0));


  checkContents(pset, prefixes);
}

function testDuplicates() {
  let pset = Cc["@mozilla.org/url-classifier/prefixset;1"]
               .createInstance(Ci.nsIUrlClassifierPrefixSet);
  let prefixes = [1,1,2,2,2,3,3,3,3,3,3,5,6,6,7,7,9,9,9];
  pset.setPrefixes(prefixes, prefixes.length);

  do_check_true(wrappedProbe(pset, 1));
  do_check_true(wrappedProbe(pset, 2));
  do_check_true(wrappedProbe(pset, 5));
  do_check_true(wrappedProbe(pset, 9));
  do_check_false(wrappedProbe(pset, 4));
  do_check_false(wrappedProbe(pset, 8));


  checkContents(pset, prefixes);
}

function testSimplePset() {
  let pset = newPset();
  let prefixes = [1,2,100,400,123456789];
  pset.setPrefixes(prefixes, prefixes.length);

  doRandomLookups(pset, prefixes, 100);
  doExpectedLookups(pset, prefixes, 1);


  checkContents(pset, prefixes);
}

function testReSetPrefixes() {
  let pset = newPset();
  let prefixes = [1, 5, 100, 1000, 150000];
  pset.setPrefixes(prefixes, prefixes.length);

  doExpectedLookups(pset, prefixes, 1);

  let secondPrefixes = [12, 50, 300, 2000, 5000, 200000];
  pset.setPrefixes(secondPrefixes, secondPrefixes.length);

  doExpectedLookups(pset, secondPrefixes, 1);
  for (let i = 0; i < prefixes.length; i++) {
    do_check_false(wrappedProbe(pset, prefixes[i]));
  }


  checkContents(pset, secondPrefixes);
}

function testLargeSet() {
  let N = 1000;
  let arr = [];

  for (let i = 0; i < N; i++) {
    let randInt = Math.floor(Math.random() * Math.pow(2, 32));
    arr.push(randInt);
  }

  arr.sort(function(x,y) x - y);

  let pset = newPset();
  pset.setPrefixes(arr, arr.length);

  doExpectedLookups(pset, arr, 1);
  doRandomLookups(pset, arr, 1000);


  checkContents(pset, arr);
}

function testTinySet() {
  let pset = Cc["@mozilla.org/url-classifier/prefixset;1"]
               .createInstance(Ci.nsIUrlClassifierPrefixSet);
  let prefixes = [1];
  pset.setPrefixes(prefixes, prefixes.length);

  do_check_true(wrappedProbe(pset, 1));
  do_check_false(wrappedProbe(pset, 100000));
  checkContents(pset, prefixes);

  prefixes = [];
  pset.setPrefixes(prefixes, prefixes.length);
  do_check_false(wrappedProbe(pset, 1));
  checkContents(pset, prefixes);
}

let tests = [testBasicPset,
             testSimplePset,
             testReSetPrefixes,
             testLargeSet,
             testDuplicates,
             testTinySet];

function run_test() {
  
  
  for (let i = 0; i < tests.length; i++) {
    dump("Running " + tests[i].name + "\n");
    tests[i]();
  }
}

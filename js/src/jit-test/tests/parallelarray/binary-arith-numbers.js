load(libdir + "parallelarray-helpers.js");





function theTest() {
  var mixedArray = [1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.1,
                    "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"];

  function op(e, i) {
    return mixedArray[e % mixedArray.length] + i;
  }

  
  
  var jsarray0 = range(0, 1024);
  jsarray0.map(op);

  
  var jsarray1 = range(0, 1024).map(i => i % 10);
  compareAgainstArray(jsarray1, "map", op);

  
  new ParallelArray(jsarray0).map(op, {mode:"par", expect:"disqualified"});
}

if (getBuildConfiguration().parallelJS)
  theTest();

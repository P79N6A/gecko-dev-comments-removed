load(libdir + "parallelarray-helpers.js");

function makeObject(e, i, c) {
  var v = {element: e, index: i, collection: c};

  if (e == 512) 
    delete v.index;

  return v;
}

function test() {
  var array = range(0, minItemsTestingThreshold);
  var array1 = array.map(makeObject);

  assertParallelExecWillRecover(function (m) {
    var pa = new ParallelArray(array);
    var pa1 = pa.map(makeObject, m);
    
  });
}

if (getBuildConfiguration().parallelJS)
  test();

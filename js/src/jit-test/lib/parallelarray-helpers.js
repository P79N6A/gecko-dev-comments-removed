


























var minItemsTestingThreshold = 1024;





var MODE_STRINGS = ["compile", "par", "seq"];
var MODES = MODE_STRINGS.map(s => ({mode: s}));

var INVALIDATE_MODE_STRINGS = ["seq", "compile", "par", "seq"];
var INVALIDATE_MODES = INVALIDATE_MODE_STRINGS.map(s => ({mode: s}));

function build(n, f) {
  var result = [];
  for (var i = 0; i < n; i++)
    result.push(f(i));
  return result;
}

function range(n, m) {
  

  var result = [];
  for (var i = n; i < m; i++)
    result.push(i);
  return result;
}

function seq_scan(array, f) {
  

  var result = [];
  result[0] = array[0];
  for (var i = 1; i < array.length; i++) {
    result[i] = f(result[i-1], array[i]);
  }
  return result;
}

function assertAlmostEq(v1, v2) {
  if (v1 === v2)
    return true;
  
  assertEq(typeof v1, "number");
  assertEq(typeof v2, "number");
  var diff = Math.abs(v1 - v2);
  var percent = diff / v1 * 100.0;
  print("v1 = " + v1);
  print("v2 = " + v2);
  print("% diff = " + percent);
  assertEq(percent < 1e-10, true); 
}

function assertStructuralEq(e1, e2) {
    if (e1 instanceof Array && e2 instanceof Array) {
      assertEqArray(e1, e2);
    } else if (e1 instanceof Object && e2 instanceof Object) {
      assertEq(e1.__proto__, e2.__proto__);
      for (prop in e1) {
        if (e1.hasOwnProperty(prop)) {
          assertEq(e2.hasOwnProperty(prop), true);
          assertStructuralEq(e1[prop], e2[prop]);
        }
      }
    } else {
      assertEq(e1, e2);
    }
}

function assertEqArray(a, b) {
    assertEq(a.length, b.length);
    for (var i = 0, l = a.length; i < l; i++) {
      try {
        assertStructuralEq(a[i], b[i]);
      } catch (e) {
        print("...in index", i, "of", l);
        throw e;
      }
    }
}














function assertParallelExecWillBail(opFunction) {
  opFunction({mode:"compile"}); 
  opFunction({mode:"bailout"}); 
}





function assertParallelExecWillRecover(opFunction) {
  opFunction({mode:"compile"}); 
  opFunction({mode:"recover"}); 
}








function assertParallelExecSucceeds(opFunction, cmpFunction) {
  var failures = 0;
  while (true) {
    print("Attempting compile #", failures);
    var result = opFunction({mode:"compile"});
    cmpFunction(result);

    try {
      print("Attempting parallel run #", failures);
      var result = opFunction({mode:"par"});
      cmpFunction(result);
      break;
    } catch (e) {
      failures++;
      if (failures > 5) {
        throw e; 
      } else {
        print(e);
      }
    }
  }

  print("Attempting sequential run");
  var result = opFunction({mode:"seq"});
  cmpFunction(result);
}











function assertArraySeqParResultsEq(arr, op, func, cmpFunc) {
  if (!cmpFunc)
    cmpFunc = assertStructuralEq;
  var expected = arr[op].apply(arr, [func]);
  assertParallelExecSucceeds(
    function (m) { return arr[op + "Par"].apply(arr, [func, m]); },
    function (r) { cmpFunc(expected, r); });
}



function testArrayScanPar(jsarray, func, cmpFunction) {
  if (!cmpFunction)
    cmpFunction = assertStructuralEq;
  var expected = seq_scan(jsarray, func);

  
  

  assertParallelExecSucceeds(
    function(m) {
      print(m.mode + " " + m.expect);
      var p = jsarray.scanPar(func, m);
      return p;
    },
    function(r) {
      cmpFunction(expected, r);
    });
}



function assertParallelModesCommute(modes, opFunction) {
  var expected = undefined;
  var acc = opFunction(modes[0]);
  assertParallelExecSucceeds(
    opFunction,
    function(r) {
      if (expected === undefined)
        expected = r;
      else
        assertStructuralEq(expected, r);
    });
}




























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
    if (e1 instanceof ParallelArray && e2 instanceof ParallelArray) {
      assertEqParallelArray(e1, e2);
    } else if (e1 instanceof Array && e2 instanceof ParallelArray) {
      assertEqParallelArrayArray(e2, e1);
    } else if (e1 instanceof ParallelArray && e2 instanceof Array) {
      assertEqParallelArrayArray(e1, e2);
    } else if (e1 instanceof Array && e2 instanceof Array) {
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

function assertEqParallelArrayArray(a, b) {
  assertEq(a.shape.length, 1);
  assertEq(a.length, b.length);
  for (var i = 0, l = a.length; i < l; i++) {
    try {
      assertStructuralEq(a.get(i), b[i]);
    } catch (e) {
      print("...in index ", i, " of ", l);
      throw e;
    }
  }
}

function assertEqArray(a, b) {
    assertEq(a.length, b.length);
    for (var i = 0, l = a.length; i < l; i++) {
      try {
        assertStructuralEq(a[i], b[i]);
      } catch (e) {
        print("...in index ", i, " of ", l);
        throw e;
      }
    }
}

function assertEqParallelArray(a, b) {
  assertEq(a instanceof ParallelArray, true);
  assertEq(b instanceof ParallelArray, true);

  var shape = a.shape;
  assertEqArray(shape, b.shape);

  function bump(indices) {
    var d = indices.length - 1;
    while (d >= 0) {
      if (++indices[d] < shape[d])
        break;
      indices[d] = 0;
      d--;
    }
    return d >= 0;
  }

  var iv = shape.map(function () { return 0; });
  do {
    try {
      var e1 = a.get.apply(a, iv);
      var e2 = b.get.apply(b, iv);
      assertStructuralEq(e1, e2);
    } catch (e) {
      print("...in indices ", iv, " of ", shape);
      throw e;
    }
  } while (bump(iv));
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











function compareAgainstArray(jsarray, opname, func, cmpFunction) {
  if (!cmpFunction)
    cmpFunction = assertStructuralEq;
  var expected = jsarray[opname].apply(jsarray, [func]);
  var parray = new ParallelArray(jsarray);
  assertParallelExecSucceeds(
    function(m) {
      return parray[opname].apply(parray, [func, m]);
    },
    function(r) {
      cmpFunction(expected, r);
    });
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





function testScatter(opFunction, cmpFunction) {
  var strategies = ["divide-scatter-version", "divide-output-range"];
  for (var i in strategies) {
    assertParallelExecSucceeds(
      function(m) {
        var m1 = {mode: m.mode,
                  strategy: strategies[i]};
        print(JSON.stringify(m1));
        return opFunction(m1);
      },
      cmpFunction);
  }
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

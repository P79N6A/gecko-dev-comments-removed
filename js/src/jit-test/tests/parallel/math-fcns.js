
load(libdir + "parallelarray-helpers.js");




var len = 5000;
var iters = 100;




















function check(fill) {
  var seq = Array.build(len, fill);
  for (var i = 0; i < iters; i++) {
    var par = Array.buildPar(len, fill);
    assertStructuralEq(par, seq);
  }
}

function checkAbs(a)   { check(function (i) { return Math.abs(a[i]);   }); }
function checkAcos(a)  { check(function (i) { return Math.acos(a[i]);  }); }
function checkAsin(a)  { check(function (i) { return Math.asin(a[i]);  }); }
function checkAtan(a)  { check(function (i) { return Math.atan(a[i]);  }); }
function checkAtan2(a) { check(function (i) { return Math.atan2(a[i]); }); }
function checkCeil(a)  { check(function (i) { return Math.ceil(a[i]);  }); }
function checkCos(a)   { check(function (i) { return Math.cos(a[i]);   }); }
function checkExp(a)   { check(function (i) { return Math.exp(a[i]);   }); }
function checkFloor(a) { check(function (i) { return Math.floor(a[i]); }); }
function checkLog(a)   { check(function (i) { return Math.log(a[i]);   }); }
function checkRound(a) { check(function (i) { return Math.round(a[i]); }); }
function checkSin(a)   { check(function (i) { return Math.sin(a[i]);   }); }
function checkSqrt(a)  { check(function (i) { return Math.sqrt(a[i]);  }); }
function checkTan(a)   { check(function (i) { return Math.tan(a[i]);   }); }

function callVariousUnaryMathFunctions() {
  
  
  
  
  
  
  function fill(i) { return 10/i; }
  var input = Array.build(len, fill);

  checkAbs(input);    
  checkAcos(input);   
  checkAsin(input);   
  checkAtan(input);   

  checkAtan2(input);  
  checkCeil(input);   
  checkCos(input);    
  checkExp(input);    

  checkFloor(input);  
  checkLog(input);    
  checkRound(input);  
  checkSin(input);    

  checkSqrt(input);   
  checkTan(input);    
}

if (getBuildConfiguration().parallelJS)
  callVariousUnaryMathFunctions();

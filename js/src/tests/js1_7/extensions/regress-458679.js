






var BUGNUMBER = 458679;
var summary = 'Do not assert: nbytes != 0';
var actual = '';
var expect = '';

printBugNumber(BUGNUMBER);
printStatus (summary);

function f()
{
  for (var i = 1; i < dps.length; ++i) {
    var a = "";
    var b = "";
    var c = "";
  }
}

function stringOfLength(n)
{
  if (n == 0) {
    return "";
  } else if (n == 1) {
    return "\"";
  } else {
    var r = n % 2;
    var d = (n - r) / 2;
    var y = stringOfLength(d);
    return y + y + stringOfLength(r);
  }    
}

try
{
  this.__defineGetter__('x', this.toSource);
  while (x.length < 12000000) { 
    let q = x;
    s = q + q; 
  }
  print(x.length);
}
catch(ex)
{
}
 
reportCompare(expect, actual, summary);

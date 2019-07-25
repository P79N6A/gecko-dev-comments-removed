

var nlocals = 50;
var localstr = "";
for (var i = 0; i < nlocals; ++i)
    localstr += "var x" + i + "; ";






var arr = [function() {return 0}, function() {return 1}, function() {return 2}];
var arg = "x";
var body = localstr +
           "if (x == 0) return; " +
           "arr[3] = (new Function(arg, body));" +
           "for (var i = 0; i < 4; ++i) arr[i](x-1);";
(new Function(arg, body))(1000);





var gotIn = false;
var threwOut = false;
try {
    (function() {
        gotIn = true;
        (new Function(arg, body))(10000000);
     }).apply(null, new Array(500 * 1024));
} catch(e) {
    assertEq(""+e, "InternalError: too much recursion");
    threwOut = true;
}
assertEq(threwOut, true);

assertEq(gotIn, true);

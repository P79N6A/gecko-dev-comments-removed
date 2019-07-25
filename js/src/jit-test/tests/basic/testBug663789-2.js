







o = { toString:function() { return evalInFrame(1, "arguments; x") } }
var s = "aaaaaaaaaa".replace(/a/g, function() { var x = 'B'; return o });
assertEq(s, "BBBBBBBBBB");

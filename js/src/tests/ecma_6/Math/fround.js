





assertEq(Math.fround(), NaN);


assertEq(Math.fround(NaN), NaN);
assertEq(Math.fround(-Infinity), -Infinity);
assertEq(Math.fround(Infinity), Infinity);
assertEq(Math.fround(-0), -0);
assertEq(Math.fround(+0), +0);


var toFloat32 = (function() {
    var f32 = new Float32Array(1);
    function f(x) {
        f32[0] = x;
        return f32[0];
    }
    return f;
})();



for (var i = 0; i < 64; ++i) {
    var p = Math.pow(2, i) + 1;
    assertEq(Math.fround(p), toFloat32(p));
    assertEq(Math.fround(-p), toFloat32(-p));
}




function maxValue(exponentWidth, significandWidth) {
    var n = 0;
    var maxExp = Math.pow(2, exponentWidth - 1) - 1;
    for (var i = significandWidth; i >= 0; i--)
        n += Math.pow(2, maxExp - i);
    return n;
}

var DBL_MAX = maxValue(11, 52);
assertEq(DBL_MAX, Number.MAX_VALUE); 


assertEq(Math.fround(DBL_MAX), Infinity);

var FLT_MAX = maxValue(8, 23);
assertEq(Math.fround(FLT_MAX), FLT_MAX);
assertEq(Math.fround(FLT_MAX + Math.pow(2, Math.pow(2, 8 - 1) - 1 - 23 - 2)), FLT_MAX); 
assertEq(Math.fround(FLT_MAX + Math.pow(2, Math.pow(2, 8 - 1) - 1 - 23 - 1)), Infinity); 





function minValue(exponentWidth, significandWidth) {
    return Math.pow(2, -(Math.pow(2, exponentWidth - 1) - 2) - significandWidth);
}

var DBL_MIN = Math.pow(2, -1074);
assertEq(DBL_MIN, Number.MIN_VALUE); 


assertEq(Math.fround(DBL_MIN), 0);

var FLT_MIN = minValue(8, 23);
assertEq(Math.fround(FLT_MIN), FLT_MIN);

assertEq(Math.fround(FLT_MIN / 2), 0); 
assertEq(Math.fround(FLT_MIN / 2 + Math.pow(2, -202)), FLT_MIN); 

assertEq(Math.fround(-FLT_MIN), -FLT_MIN);

assertEq(Math.fround(-FLT_MIN / 2), -0); 
assertEq(Math.fround(-FLT_MIN / 2 - Math.pow(2, -202)), -FLT_MIN); 

reportCompare(0, 0, "ok");

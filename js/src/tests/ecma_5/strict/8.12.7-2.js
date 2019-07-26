







var hits = 0;
var p = {
    toString: function () {
        hits++;
        return "noconfig";
    }
};
assertEq(testLenientAndStrict('var o = Object.freeze({noconfig: "ow"}); delete o[p]',
                              returns(false), raisesException(TypeError)),
         true);
assertEq(hits, 2);

reportCompare(true, true);






function TestManySmallArrays() {
    function doIter(f, arr) {
        return f(...new Set(arr));
    }

    function fun(a, b, c) {
        var result = 0;
        for (var i = 0; i < arguments.length; i++)
            result += arguments[i];
        return result;
    }


    var TRUE_SUM = 0;
    var N = 100;
    var M = 3;
    var sum = 0;
    for (var i = 0; i < N; i++) {
        var arr = new Array(M);
        for (var j = 0; j < M; j++) {
            arr[j] = j;
            TRUE_SUM += j;
        }
        sum += doIter(fun, arr);
    }
    assertEq(sum, TRUE_SUM);
}
TestManySmallArrays();


function TestSingleSmallArray() {
    function doIter(f, arr) {
        return f(...new Set(arr));
    }

    function fun(a, b, c) {
        var result = 0;
        for (var i = 0; i < arguments.length; i++)
            result += arguments[i];
        return result;
    }


    var TRUE_SUM = 0;
    var N = 100;
    var M = 3;
    var arr = new Array(M);
    for (var j = 0; j < M; j++) {
        arr[j] = j;
        TRUE_SUM += j;
    }
    TRUE_SUM *= N;

    var sum = 0;
    for (var i = 0; i < N; i++) {
        sum += doIter(fun, arr);
    }
    assertEq(sum, TRUE_SUM);
}
TestSingleSmallArray();


function TestChangeArrayPrototype() {
    function doIter(f, arr) {
        return f(...new Set(arr));
    }

    function fun(a, b, c) {
        var result = 0;
        for (var i = 0; i < arguments.length; i++)
            result += arguments[i];
        return result;
    }

    var Proto1 = Object.create(Array.prototype);

    var TRUE_SUM = 0;
    var N = 100;
    var MID = N/2;
    var M = 3;
    var arr = new Array(M);
    var ARR_SUM = 0;
    for (var j = 0; j < M; j++) {
        arr[j] = j;
        ARR_SUM += j;
    }

    var sum = 0;
    for (var i = 0; i < N; i++) {
        sum += doIter(fun, arr);
        if (i == MID)
            arr.__proto__ = Proto1;
        TRUE_SUM += ARR_SUM;
    }
    assertEq(sum, TRUE_SUM);
}
TestChangeArrayPrototype();


function TestChangeManyArrayShape() {
    function doIter(f, arr) {
        return f(...new Set(arr));
    }

    function fun(a, b, c) {
        var result = 0;
        for (var i = 0; i < arguments.length; i++)
            result += arguments[i];
        return result;
    }

    var TRUE_SUM = 0;
    var N = 100;
    var MID = N/2;
    var M = 3;
    var sum = 0;
    for (var i = 0; i < N; i++) {
        var arr = new Array(M);
        var ARR_SUM = 0;
        for (var j = 0; j < M; j++) {
            arr[j] = j;
            ARR_SUM += j;
        }
        arr['v_' + i] = i;
        sum += doIter(fun, arr);
        TRUE_SUM += ARR_SUM;
    }
    assertEq(sum, TRUE_SUM);
}
TestChangeManyArrayShape();

if (typeof reportCompare === "function")
  reportCompare(true, true);

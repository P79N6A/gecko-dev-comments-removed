






function TestDecreaseArrayLength() {
    function doIter(f, arr) {
        return f(...new Set(arr));
    }

    function fun(a, b, c) {
        var result = 0;
        for (var i = 0; i < arguments.length; i++)
            result += arguments[i];
        return result;
    }

    var GET_COUNT = 0;
    function getter() {
        GET_COUNT++;
        if (GET_COUNT == MID) {
            arr.length = 0;
        }
        return M2;
    }

    var iter = ([])['@@iterator']();
    var iterProto = Object.getPrototypeOf(iter);
    var OldNext = iterProto.next;
    var NewNext = function () {
        return OldNext.apply(this, arguments);
    };

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
    var M2 = (M/2)|0;
    Object.defineProperty(arr, M2, {'get':getter});

    var sum = 0;
    for (var i = 0; i < N; i++) {
        var oldLen = arr.length;
        sum += doIter(fun, arr);
        var newLen = arr.length;
        if (oldLen == newLen)
            TRUE_SUM += arr.length > 0 ? ARR_SUM : 0;
        else
            TRUE_SUM += 1
    }
    assertEq(sum, TRUE_SUM);
}
TestDecreaseArrayLength();

if (typeof reportCompare === "function")
  reportCompare(true, true);

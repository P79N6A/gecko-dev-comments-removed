


(function() {
    var Y = function() {};
    function g() {
        function f(x) {
            for (var j = 0; j < 1; ++j) {
                x.apply(this);
            }
        }
        return function() {
            f(Y);
        }
    };
    for (var i = 0; i < 2; ++i) {
        g()();
    }
})();

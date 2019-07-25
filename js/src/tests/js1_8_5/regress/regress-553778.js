





function f() {
    function g() {
        function h() {
            g; x;
        }
        var [x] = [];
    }
}

reportCompare(true, true);

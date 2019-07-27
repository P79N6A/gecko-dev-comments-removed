

assertThrowsInstanceOf(() =>
({
    method() {
        (function () {
            eval('super.toString');
        })();
    }
}).method(), SyntaxError);

if (typeof reportCompare === 'function')
    reportCompare(0,0,"OK");

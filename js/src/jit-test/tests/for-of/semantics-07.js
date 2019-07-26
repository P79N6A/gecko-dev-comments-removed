


load(libdir + "asserts.js");
load(libdir + "iteration.js");

var iterProto = Object.getPrototypeOf([][std_iterator]());
var s = '';
assertThrowsInstanceOf(function () {
    for (var v of ['duck', 'duck', 'duck', 'goose', 'FAIL']) {
        s += v;
        if (v === 'goose')
            delete iterProto.next;
        s += '.';
    }
}, TypeError);
assertEq(s, 'duck.duck.duck.goose.');

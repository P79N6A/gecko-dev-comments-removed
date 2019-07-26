



options("strict_mode");
function test(str, arg, result) {
    var fun = new Function('x', str);
    var got = fun.toSource();
}
test('return let (y) x;');



function test(s) {
    var copy = '';
    for (var v of s) {
        assertEq(typeof v, 'string');
        assertEq(v.length, 1);
        copy += v;
    }
    assertEq(copy, String(s));
}

test('');
test('abc');
test('a \0 \ufffe \ufeff');





test('\ud808\udf45');

test(new String(''));
test(new String('abc'));
test(new String('a \0 \ufffe \ufeff'));
test(new String('\ud808\udf45'));

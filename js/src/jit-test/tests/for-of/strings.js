

load(libdir + "string.js");

function test(s, expectedCodePoints) {
    var copy = '';
    var codepoints = 0;
    var singleHighSurrogate = false;
    for (var v of s) {
        assertEq(typeof v, 'string');
        assertEq(v.length, isSurrogatePair(v) ? 2 : 1);
        assertEq(false, singleHighSurrogate && isLowSurrogate(v));
        copy += v;
        codepoints += 1;
        singleHighSurrogate = !isSurrogatePair(v) && isHighSurrogate(v);
    }
    assertEq(copy, String(s));
    assertEq(codepoints, expectedCodePoints);
}

test('', 0);
test('abc', 3);
test('a \0 \ufffe \ufeff', 7);



test('\ud808\udf45', 1);



test('\ud808', 1);
test('\ud808\u0000', 2);

test('\udf45', 1);
test('\u0000\udf45', 2);

test('\udf45\ud808', 2);

test(new String(''), 0);
test(new String('abc'), 3);
test(new String('a \0 \ufffe \ufeff'), 7);
test(new String('\ud808\udf45'), 1);
test(new String('\ud808'), 1);
test(new String('\ud808\u0000'), 2);
test(new String('\udf45'), 1);
test(new String('\u0000\udf45'), 2);
test(new String('\udf45\ud808'), 2);

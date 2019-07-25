
























var BUGNUMBER = 614608;
var summary = "String.prototype.split with regexp separator";

print(BUGNUMBER + ": " + summary);





var ecmaSampleRe = /<(\/)?([^<>]+)>/;

var testCode = [
    ["''.split()",                   [""]],
    ["''.split(/./)",                [""]],
    ["''.split(/.?/)",               []],
    ["''.split(/.??/)",              []],
    ["'ab'.split(/a*/)",             ["", "b"]],
    ["'ab'.split(/a*?/)",            ["a", "b"]],
    ["'ab'.split(/(?:ab)/)",         ["", ""]],
    ["'ab'.split(/(?:ab)*/)",        ["", ""]],
    ["'ab'.split(/(?:ab)*?/)",       ["a", "b"]],
    ["'test'.split('')",             ["t", "e", "s", "t"]],
    ["'test'.split()",               ["test"]],
    ["'111'.split(1)",               ["", "", "", ""]],
    ["'test'.split(/(?:)/, 2)",      ["t", "e"]],
    ["'test'.split(/(?:)/, -1)",     ["t", "e", "s", "t"]],
    ["'test'.split(/(?:)/, undefined)", ["t", "e", "s", "t"]],
    ["'test'.split(/(?:)/, null)",   []],
    ["'test'.split(/(?:)/, NaN)",    []],
    ["'test'.split(/(?:)/, true)",   ["t"]],
    ["'test'.split(/(?:)/, '2')",    ["t", "e"]],
    ["'test'.split(/(?:)/, 'two')",  []],
    ["'a'.split(/-/)",               ["a"]],
    ["'a'.split(/-?/)",              ["a"]],
    ["'a'.split(/-??/)",             ["a"]],
    ["'a'.split(/a/)",               ["", ""]],
    ["'a'.split(/a?/)",              ["", ""]],
    ["'a'.split(/a??/)",             ["a"]],
    ["'ab'.split(/-/)",              ["ab"]],
    ["'ab'.split(/-?/)",             ["a", "b"]],
    ["'ab'.split(/-??/)",            ["a", "b"]],
    ["'a-b'.split(/-/)",             ["a", "b"]],
    ["'a-b'.split(/-?/)",            ["a", "b"]],
    ["'a-b'.split(/-??/)",           ["a", "-", "b"]],
    ["'a--b'.split(/-/)",            ["a", "", "b"]],
    ["'a--b'.split(/-?/)",           ["a", "", "b"]],
    ["'a--b'.split(/-??/)",          ["a", "-", "-", "b"]],
    ["''.split(/()()/)",             []],
    ["'.'.split(/()()/)",            ["."]],
    ["'.'.split(/(.?)(.?)/)",        ["", ".", "", ""]],
    ["'.'.split(/(.??)(.??)/)",      ["."]],
    ["'.'.split(/(.)?(.)?/)",        ["", ".", undefined, ""]],
    ["'A<B>bold</B>and<CODE>coded</CODE>'.split(ecmaSampleRe)",
                                     ["A", undefined, "B", "bold", "/", "B",
                                      "and", undefined, "CODE", "coded", "/",
                                      "CODE", ""]],
    ["'tesst'.split(/(s)*/)",        ["t", undefined, "e", "s", "t"]],
    ["'tesst'.split(/(s)*?/)",       ["t", undefined, "e", undefined, "s",
                                      undefined, "s", undefined, "t"]],
    ["'tesst'.split(/(s*)/)",        ["t", "", "e", "ss", "t"]],
    ["'tesst'.split(/(s*?)/)",       ["t", "", "e", "", "s", "", "s", "", "t"]],
    ["'tesst'.split(/(?:s)*/)",      ["t", "e", "t"]],
    ["'tesst'.split(/(?=s+)/)",      ["te", "s", "st"]],
    ["'test'.split('t')",            ["", "es", ""]],
    ["'test'.split('es')",           ["t", "t"]],
    ["'test'.split(/t/)",            ["", "es", ""]],
    ["'test'.split(/es/)",           ["t", "t"]],
    ["'test'.split(/(t)/)",          ["", "t", "es", "t", ""]],
    ["'test'.split(/(es)/)",         ["t", "es", "t"]],
    ["'test'.split(/(t)(e)(s)(t)/)", ["", "t", "e", "s", "t", ""]],
    ["'.'.split(/(((.((.??)))))/)",  ["", ".", ".", ".", "", "", ""]],
    ["'.'.split(/(((((.??)))))/)",   ["."]]
];

function testSplit() {
    for (var i = 0; i < testCode.length; i++) {
        var actual = eval(testCode[i][0]);
        var expected = testCode[i][1];

        assertEq(actual.length, expected.length);

        for(var j=0; j<actual.length; j++) {
            assertEq(actual[j], expected[j], testCode[i][0]);
        }
    }
}

testSplit();

if (typeof reportCompare === "function")
    reportCompare(true, true);

print("All tests passed!");

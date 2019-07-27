










function syntaxError (script) {
    try {
        Function(script);
    } catch (e) {
        if (e.name === "SyntaxError") {
            return;
        }
    }
    throw "Expected syntax error: " + script;
}




syntaxError("`");
syntaxError("`$");
syntaxError("`${");
syntaxError("`${}");
syntaxError("`${1}");
syntaxError("`${1 + 2}");


assertEq("$", `$`);
assertEq("$}", `$}`);
assertEq("}", `}`);
assertEq("{", `{`);



assertEq("\'", `\'`);
assertEq("\"", `\"`);
assertEq("\\", `\\`);
assertEq("\b", `\b`);
assertEq("\f", `\f`);
assertEq("\n", `\n`);
assertEq("\r", `\r`);
assertEq("\t", `\t`);
assertEq("\v", `\v`);
assertEq("\r\n", `\r\n`);


assertEq("\0", eval("`\\" + String.fromCharCode(0) + "`"));
assertEq("$", `\$`);
assertEq(".", `\.`);
assertEq("A", `\A`);
assertEq("a", `\a`);



assertEq("\0", `\0`);
syntaxError("`\\1`");
syntaxError("`\\2`");
syntaxError("`\\3`");
syntaxError("`\\4`");
syntaxError("`\\5`");
syntaxError("`\\6`");
syntaxError("`\\7`");
syntaxError("`\\01`");
syntaxError("`\\001`");
syntaxError("`\\00`");


syntaxError("`\\x`");
syntaxError("`\\x0`");
syntaxError("`\\x0Z`");
syntaxError("`\\xZ`");

assertEq("\0", `\x00`);
assertEq("$", `\x24`);
assertEq(".", `\x2E`);
assertEq("A", `\x41`);
assertEq("a", `\x61`);
assertEq("AB", `\x41B`);
assertEq(String.fromCharCode(0xFF), `\xFF`);




assertEq("\0", `\u0000`);
assertEq("$", `\u0024`);
assertEq(".", `\u002E`);
assertEq("A", `\u0041`);
assertEq("a", `\u0061`);
assertEq("AB", `\u0041B`);
assertEq(String.fromCharCode(0xFFFF), `\uFFFF`);



assertEq("", eval("`\\\n`"));
assertEq("", eval("`\\\r`"));
assertEq("", eval("`\\\u2028`"));
assertEq("", eval("`\\\u2029`"));
assertEq("\u2028", eval("`\u2028`"));
assertEq("\u2029", eval("`\u2029`"));

assertEq("a\nb", eval("`a\rb`"))
assertEq("a\nb", eval("`a\r\nb`"))
assertEq("a\n\nb", eval("`a\r\rb`"))



for (var i = 0; i < 0xFF; ++i) {
    var c = String.fromCharCode(i);
    if (c == "`" || c == "\\" || c == "\r") continue;
    assertEq(c, eval("`" + c + "`"));
}

assertEq("", ``);
assertEq("`", `\``);
assertEq("$", `$`);
assertEq("$$", `$$`);
assertEq("$$}", `$$}`);


assertEq(`hey
there`, "hey\nthere");


syntaxError("var obj = { `illegal`: 1}");


assertThrowsInstanceOf(() => JSON.parse('[1, `false`]'), SyntaxError);

syntaxError('({get `name`() { return 10; }});');


assertEq(5, Function("`use strict`; return 05;")());
var func = function f() {
    `ignored string`;
    "use strict";
    return 06;
}
assertEq(6, func());
syntaxError("\"use strict\"; return 06;");


reportCompare(0, 0, "ok");

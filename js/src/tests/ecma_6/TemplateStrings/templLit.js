










function syntaxError (script) {
    try {
        Function(script);
    } catch (e) {
        if (e.name === "SyntaxError") {
            return;
        }
    }
    throw new Error('Expected syntax error: ' + script);
}






assertEq("abcdef", `ab${"cd"}ef`);
assertEq("ab9ef", `ab${4+5}ef`);
assertEq("cdef", `${"cd"}ef`);
assertEq("abcd", `ab${"cd"}`);
assertEq("cd", `${"cd"}`);
assertEq("", `${""}`);
assertEq("4", `${4}`);


assertEq("abcdef", `ab${"cd"}e${"f"}`);
assertEq("abcdef", `ab${"cd"}${"e"}f`);
assertEq("abcdef", `a${"b"}${"cd"}e${"f"}`);
assertEq("abcdef", `${"ab"}${"cd"}${"ef"}`);


assertEq("abcdef", `a${`b${"cd"}e${"f"}`}`);

syntaxError("`${}`");
syntaxError("`${`");
syntaxError("`${\\n}`");
syntaxError("`${yield 0}`");


assertEq(`${
0
}`, "0");

assertEq(`${ 


0}`, "0");


syntaxError("`${0;}`");
assertEq(`${{}}`, "[object Object]");
assertEq(`${
    function f() {
        return "ok";
    }()
}`, "ok");


var x = 0;
assertEq(`= ${x += 1}`, "= 1");
assertEq(x, 1);



x = 0;
assertEq(`${++x, "o"}k`, "ok");
assertEq(x, 1);


assertEq(`
--> this is text
`, "\n--> this is text\n");


function f(n) {
    if (n === 0)
        return "";
    return `${n}${f(n - 1)}`;
}
assertEq(f(9), "987654321");


function* g() {
    return `${yield 1} ${yield 2}`;
}

var it = g();
var next = it.next();
assertEq(next.done, false);
assertEq(next.value, 1);
next = it.next("hello");
assertEq(next.done, false);
assertEq(next.value, 2);
next = it.next("world");
assertEq(next.done, true);
assertEq(next.value, "hello world");


assertEq(`${void 0}`, "undefined");
assertEq(`${Object.doesNotHaveThisProperty}`, "undefined");


assertEq("<toString>", `${{valueOf: () => "<valueOf>", toString: () => "<toString>"}}`);
assertEq("Hi 42", Function("try {`${{toString: () => { throw 42;}}}`} catch(e) {return \"Hi \" + e;}")());


reportCompare(0, 0, "ok");

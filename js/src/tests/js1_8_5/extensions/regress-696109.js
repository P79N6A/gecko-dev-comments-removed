







try {
    Reflect.parse("with({foo})bar");
    throw new Error("supposed to be a syntax error");
} catch (e if e instanceof SyntaxError) { }
try {
    Reflect.parse("while({foo})bar");
    throw new Error("supposed to be a syntax error");
} catch (e if e instanceof SyntaxError) { }

reportCompare(true, true);

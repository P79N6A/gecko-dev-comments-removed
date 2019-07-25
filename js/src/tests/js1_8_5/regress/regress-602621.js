





function test(arg) {
    eval(arg);
    {
        function arguments() { return 1; }
    }
    return arguments;
}

reportCompare("function", typeof test("42"), "function sub-statement must override arguments");






function f(a, b, c, d) {
    yield arguments.length;
}
reportCompare(0, f().next(), "bug 530879");

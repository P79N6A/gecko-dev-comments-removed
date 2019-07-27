
function f0(a, a) {
}


assertThrowsInstanceOf(() => eval(`
function f1(a, ...a) {
}
`), SyntaxError);


assertThrowsInstanceOf(() => eval(`
function f2(a, a, ...b) {
}
`), SyntaxError);

reportCompare(0, 0, 'ok');


function f0(a, a) {
}


assertThrowsInstanceOf(() => eval(`
(a, a) => {
};
`), SyntaxError);
assertThrowsInstanceOf(() => eval(`
(a, ...a) => {
};
`), SyntaxError);

reportCompare(0, 0, 'ok');

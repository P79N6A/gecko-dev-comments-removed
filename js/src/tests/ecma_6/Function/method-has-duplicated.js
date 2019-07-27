
function f0(a) {
}


assertThrowsInstanceOf(() => eval(`
({
  m1(a, a) {
  }
});
`), SyntaxError);
assertThrowsInstanceOf(() => eval(`
({
  m2(a, ...a) {
  }
});
`), SyntaxError);

reportCompare(0, 0, 'ok');

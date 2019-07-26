



function makeGenerator() {
  yield function generatorClosure() {};
}
var generator = makeGenerator();
if (typeof findReferences == 'function') {
  findReferences(generator);
}

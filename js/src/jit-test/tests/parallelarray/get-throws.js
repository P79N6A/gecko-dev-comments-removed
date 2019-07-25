

function testGetThrows() {
  
  var p = new ParallelArray([1,2,3,4]);
  p.get(42);
}

testGetThrows();

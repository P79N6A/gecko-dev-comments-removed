


function
 eval( ) { eval();}
try {
DoWhile_3();
} catch(e) {
}
function DoWhile_3() {
  var result2 = "pass";
woohooboy: {
    eval('(function() { x getter= function(){} ; var x5, x = 0x99; })();');
  }
}
test();
function test()
{
  function foopy()
  {
    var f = function(){ r = arguments; test(); yield 170; }
    try { for (var i in f()) { } } catch (iterError) { }
  }
  foopy();
  gc();
}

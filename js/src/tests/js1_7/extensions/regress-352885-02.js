





var BUGNUMBER = 352885;
var summary = 'Do not crash iterating over gen.__proto__';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function dotest()
  {
    var proto = (function() { yield 3; })().__proto__;

    try {
      proto.next();
      throw "generatorProto.next() does not throw TypeError";
    } catch (e) {
      if (!(e instanceof TypeError))
        throw "generatorProto.next() throws unexpected exception: "+uneval(e);
    }

    try {
      proto.send();
      throw "generatorProto.send() does not throw TypeError";
    } catch (e) {
      if (!(e instanceof TypeError))
        throw "generatorProto.send() throws unexpected exception: "+uneval(e);
    }

    var obj = {};
    try {
      proto.throw(obj);
      throw "generatorProto.throw(obj) does not throw TypeError";
    } catch (e) {
      if (!(e instanceof TypeError))
        throw "generatorProto.throw() throws unexpected exception: "+uneval(e);
    }

    try {
      proto.close();
      throw "generatorProto.close() does not throw TypeError";
    } catch (e) {
      if (!(e instanceof TypeError))
        throw "generatorProto.close() throws unexpected exception: "+uneval(e);
    }

  }

  dotest();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

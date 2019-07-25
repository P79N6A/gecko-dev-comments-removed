






































var BUGNUMBER = 352846;
var summary = 'Passing unrooted value to OBJ_DEFAULT_VALUE is GC hazard';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
START(summary);

var counter = 0;

function prepare_xml()
{
  delete XML.prototype.function::toString;
  Object.defineProperty(Object.prototype, "toString",
                        { get: toSource_getter, enumerable: true,
                          configurable: true });
  return new XML("<a>xml_text</a>");
}

function toSource_getter()
{
  
  var tmp = { toSource: function() { return [1,2,3]; } };
  uneval(tmp);

  if (counter++ < 2) return undefined;

  
  if (typeof gc == "function") gc();
  var x = 1e-100;
  for (var i = 0; i != 50000; ++i)
    var x2 = x / 4;

  
  
  return function() {
    print("xxx");
    return this.toXMLString();
  };
}

var obj = { };
Object.defineProperty(obj, "length",
{
  get: prepare_xml,
  enumerable: true,
  configurable: true
});
Array.reverse(obj);

TEST(1, expect, actual);

END();

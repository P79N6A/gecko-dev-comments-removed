




































var gTestfile = 'basic-for-in.js';

var BUGNUMBER     = "346582";
var summary = "Basic support for iterable objects and for-in";
var actual, expect;

printBugNumber(BUGNUMBER);
printStatus(summary);





var failed = false;

var iterable = { persistedProp: 17 };

try
{
  
  for (var i in iterable)
  {
    if (i != "persistedProp")
      throw "no persistedProp!";
    if (iterable[i] != 17)
      throw "iterable[\"persistedProp\"] == 17";
  }

  var keys = ["foo", "bar", "baz"];
  var vals = [6, 5, 14];

  iterable.__iterator__ =
    function(keysOnly)
    {
      var gen =
      function()
      {
	for (var i = 0; i < keys.length; i++)
	{
	  if (keysOnly)
	    yield keys[i];
	  else
	    yield [keys[i], vals[i]];
	}
      };
      return gen();
    };

  
  var index = 0;
  for (var k in iterable)
  {
    if (k != keys[index])
      throw "for-in iteration failed on keys[\"" + index + "\"]";
    index++;
  }
  if (index != keys.length)
    throw "not everything iterated!  index=" + index +
      ", keys.length=" + keys.length;

  if (iterable.persistedProp != 17)
    throw "iterable.persistedProp not persisted!";
}
catch (e)
{
  failed = e;
}



expect = false;
actual = failed;

reportCompare(expect, actual, summary);

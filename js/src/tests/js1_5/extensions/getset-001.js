







































var gTestfile = 'getset-001.js';
   
function TestObject ()
{
  
}

TestObject.prototype._y = "<initial y>";
   
TestObject.prototype.y getter =
  function get_y ()
{
  var rv;
   
  if (typeof this._y == "string")
    rv = "got " + this._y;
  else
    rv = this._y;
   
  return rv;
}

  TestObject.prototype.y setter =
  function set_y (newVal)
{
  this._y = newVal;
}

    test(new TestObject());

function test(t)
{
  enterFunc ("test");
   
  printStatus ("Basic Getter/ Setter test");
  reportCompare ("<initial y>", t._y, "y prototype check");

  reportCompare ("got <initial y>", t.y, "y getter, before set");

  t.y = "new y";
  reportCompare ("got new y", t.y, "y getter, after set");

  t.y = 2;
  reportCompare (2, t.y, "y getter, after numeric set");

  var d = new Date();
  t.y = d;
  reportCompare (d, t.y, "y getter, after date set");

}

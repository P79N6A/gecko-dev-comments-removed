




function p() { }

function test()
{
  var counter = 0;

  function f(x) {
      try
      { 
        throw 42;
      }
      catch(e)
      { 
        assertEq(counter, 0);
        p(function(){x;});
        counter = 1;
      }
  }

  f(2);
  assertEq(counter, 1);
}

test();

reportCompare(0, 0, "ok");

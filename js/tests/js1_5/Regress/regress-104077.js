





























































var gTestfile = 'regress-104077.js';
var UBound = 0;
var BUGNUMBER = 104077;
var summary = "Just testing that we don't crash on with/finally/return -";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function addValues(obj)
{
  var sum;
  with (obj)
  {
    try
    {
      sum = arg1 + arg2;
    }
    finally
    {
      return sum;
    }
  }
}

status = inSection(1);
var obj = new Object();
obj.arg1 = 1;
obj.arg2 = 2;
actual = addValues(obj);
expect = 3;
captureThis();



function tryThis()
{
  var sum = 4 ;
  var i = 0;

  while (sum < 10)
  {
    try
    {
      sum += 1;
      i += 1;
    }
    finally
    {
      print("In finally case of tryThis() function");
    }
  }
  return i;
}

status = inSection(2);
actual = tryThis();
expect = 6;
captureThis();



function myTest(x)
{
  var obj = new Object();
  var msg;

  with (obj)
  {
    msg = (x != null) ? "NO" : "YES";
    print("Is the provided argument to myTest() null? : " + msg);

    try
    {
      throw "ZZZ";
    }
    catch(e)
    {
      print("Caught thrown exception = " + e);
    }
  }

  return 1;
}

status = inSection(3);
actual = myTest(null);
expect = 1;
captureThis();



function addValues_2(obj)
{
  var sum = 0;
  with (obj)
  {
    try
    {
      sum = arg1 + arg2;
      with (arg3)
      {
        while (sum < 10)
        {
          try
          {
            if (sum > 5)
              return sum;
            sum += 1;
          }
          catch(e)
          {
            print('Caught an exception in addValues_2() function: ' + e);
          }
        }
      }
    }
    finally
    {
      return sum;
    }
  }
}

status = inSection(4);
obj = new Object();
obj.arg1 = 1;
obj.arg2 = 2;
obj.arg3 = new Object();
obj.arg3.a = 10;
obj.arg3.b = 20;
actual = addValues_2(obj);
expect = 6;
captureThis();



status = inSection(5);
try
{
  throw new A();
}
catch(e)
{
}
finally
{
  try
  {
    throw new A();
  }
  catch(e)
  {
  }
  finally
  {
    actual = 'a';
  }
  actual = 'b';
}
expect = 'b';
captureThis();




function testfunc(mode)
{
  var obj = new Object();
  with (obj)
  {
    var num = 100;
    var str = "abc" ;

    if (str == null)
    {
      try
      {
        throw "authentication.0";
      }
      catch(e)
      {
      }
      finally
      {
      }

      return num;
    }
    else
    {
      try
      {
        if (mode == 0)
          throw "authentication.0";
        else
          mytest();
      }
      catch(e)
      {
      }
      finally
      {
      }

      return num;
    }
  }
}

status = inSection(6);
actual = testfunc(0);
expect = 100;
captureThis();

status = inSection(7);
actual = testfunc();
expect = 100; 
captureThis();




function entry_menu()
{
  var document = new Object();
  var dialog = new Object();
  var num = 100;

  with (document)
  {
    with (dialog)
    {
      try
      {
        while (true)
        {
          return num;
        }
      }
      finally
      {
      }
    }
  }
}

status = inSection(8);
actual = entry_menu();
expect = 100;
captureThis();




function addValues_5(obj)
{
  var sum = 0;

  with (obj)
  {
    try
    {
      sum = arg1 + arg2;
      with (arg3)
      {
        while (sum < 10)
        {
          try
          {
	    if (sum > 5)
	      return sum;
	    sum += 1;
          }
          catch (e)
          {
            sum += 1;
            print(e);
          }
        }
      }
    }
    finally
    {
      try
      {
        sum += 1;
        print("In finally block of addValues_5() function: sum = " + sum);
      }
      catch (e)
      {
        sum += 1;
        print("In finally catch block of addValues_5() function: sum = " + sum + ", e = " + e);
      }
      finally
      {
        sum += 1;
        print("In finally finally block of addValues_5() function: sum = " + sum);
        return sum;
      }
    }
  }
}

status = inSection(11);
obj = new Object();
obj.arg1 = 1;
obj.arg2 = 2;
obj.arg3 = new Object();
obj.arg3.a = 10;
obj.arg3.b = 20;
actual = addValues_5(obj);
expect = 8;
captureThis();




function testObj(obj)
{
  var x = 42;

  try
  {
    with (obj)
    {
      if (obj.p)
        throw obj.p;
      x = obj.q;
    }
  }
  finally
  {
    print("in finally block of testObj() function");
    return 999;
  }
}

status = inSection(12);
obj = {p:43};
actual = testObj(obj);
expect = 999;
captureThis();






function a120571()
{
  while(0)
  {
    try
    {
    }
    catch(e)
    {
      continue;
    }
  }
}


print(a120571);


status = inSection(13);
try
{
  actual = a120571.toString().match(/continue/)[0];
}
catch(e)
{
  actual = 'FAILED! Did not find "continue" in function body';
}
expect = 'continue';
captureThis();




function b()
{
  for(;;)
  {
    try
    {
    }
    catch(e)
    {
      continue;
    }
  }
}


print(b);


status = inSection(14);
try
{
  actual = b.toString().match(/continue/)[0];
}
catch(e)
{
  actual = 'FAILED! Did not find "continue" in function body';
}
expect = 'continue';
captureThis();






test();




function captureThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}

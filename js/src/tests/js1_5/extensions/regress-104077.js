





























































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


function addValues_3(obj)
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
        sum +=1;
        print("In finally block of addValues_3() function: sum = " + sum);
      }
      catch (e if e == 42)
      {
        sum +=1;
        print('In finally catch block of addValues_3() function: sum = ' + sum + ', e = ' + e);
      }
      finally
      {
        sum +=1;
        print("In finally finally block of addValues_3() function: sum = " + sum);
        return sum;
      }
    }
  }
}

status = inSection(9);
obj = new Object();
obj.arg1 = 1;
obj.arg2 = 2;
obj.arg3 = new Object();
obj.arg3.a = 10;
obj.arg3.b = 20;
actual = addValues_3(obj);
expect = 8;
captureThis();




function addValues_4(obj)
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
        print("In finally block of addValues_4() function: sum = " + sum);
      }
      catch (e if e == 42)
      {
        sum += 1;
        print("In 1st finally catch block of addValues_4() function: sum = " + sum + ", e = " + e);
      }
      catch (e if e == 43)
      {
        sum += 1;
        print("In 2nd finally catch block of addValues_4() function: sum = " + sum + ", e = " + e);
      }
      finally
      {
        sum += 1;
        print("In finally finally block of addValues_4() function: sum = " + sum);
        return sum;
      }
    }
  }
}

status = inSection(10);
obj = new Object();
obj.arg1 = 1;
obj.arg2 = 2;
obj.arg3 = new Object();
obj.arg3.a = 10;
obj.arg3.b = 20;
actual = addValues_4(obj);
expect = 8;
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

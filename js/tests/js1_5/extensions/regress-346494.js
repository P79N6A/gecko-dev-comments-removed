




































var gTestfile = 'regress-346494.js';

var BUGNUMBER = 346494;
var summary = 'try-catch-finally scope';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  function g()
  {
    try
    {
      throw "foo";
    }
    catch(e if e == "bar")
    {
    }
    catch(e if e == "baz")
    {
    }
    finally
    {
    }
  }

  expect = "foo";
  try
  {
    g();
    actual = 'No Exception';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  expect =
    'function g() {\n' +
    '    try {\n' +
    '        throw "foo";\n' +
    '    } catch (e if e == "bar") {\n' +
    '    } catch (e if e == "baz") {\n' +
    '    } finally {\n' +
    '    }\n' +
    '}';

  actual = g + '';
  reportCompare(expect, actual, summary);

  function h()
  {
    try
    {
      throw "foo";
    }
    catch(e if e == "bar")
    {
    }
    catch(e)
    {
    }
    finally
    {
    }
  }

  expect = "No Exception";
  try
  {
    h();
    actual = 'No Exception';
  }
  catch(ex)
  {
    actual = ex + '';
  }
  reportCompare(expect, actual, summary);

  expect =
    'function h() {\n' +
    '    try {\n' +
    '        throw "foo";\n' +
    '    } catch (e if e == "bar") {\n' +
    '    } catch (e) {\n' +
    '    } finally {\n' +
    '    }\n' +
    '}';

  actual = h + '';
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}

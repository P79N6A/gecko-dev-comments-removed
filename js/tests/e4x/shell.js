











































function reportFailure (section, msg)
{
    msg = inSection(section)+"\n"+msg;
    var lines = msg.split ("\n");
    for (var i=0; i<lines.length; i++)
        print (FAILED + lines[i]);
}

function toPrinted(value)
{
  if (typeof value == "xml") {
    return value.toXMLString();
  } else {
    return String(value);
  }
}

function START(summary)
{
  printStatus(summary);
}

function BUG(bug)
{
  printBugNumber(bug);
}

function TEST(section, expected, actual)
{
    var expected_t = typeof expected;
    var actual_t = typeof actual;
    var output = "";
    
    if (expected_t != actual_t)
        output += "Type mismatch, expected type " + expected_t + 
            ", actual type " + actual_t + "\n";
    else if (VERBOSE)
        printStatus ("Expected type '" + actual_t + "' matched actual " +
                     "type '" + expected_t + "'");

    if (expected != actual) {
	output += "Expected value:\n" + toPrinted(expected) + "\nActual value:\n" + toPrinted(actual) + "\n";
    }
    else if (VERBOSE)
        printStatus ("Expected value '" + toPrinted(actual) + "' matched actual value");

    if (output != "")
    {
        reportFailure (section, output);   
        return false;
    }
    else
    {
        print('PASSED! ' + section);
    }
    return true;
}

function TEST_XML(section, expected, actual)
{
  var actual_t = typeof actual;
  var expected_t = typeof expected;

  if (actual_t != "xml") {
    
    return TEST(section, new XML(), actual);
  }
  
  if (expected_t == "string") {
    return TEST(section, expected, actual.toXMLString());
  }

  if (expected_t == "number") {
    return TEST(section, String(expected), actual.toXMLString());
  }

  reportFailure (section, "Bad TEST_XML usage: type of expected is "+expected_t+", should be number or string");
  return false;
}

function SHOULD_THROW(section)
{
  reportFailure(section, "Expected to generate exception, actual behavior: no exception was thrown");   
}

function END()
{
}

function compareSource(n, expect, actual)
{
    
    var expectP = expect.
        replace(/([(){},.:\[\]])/mg, ' $1 ').
        replace(/(\w+)/mg, ' $1 ').
        replace(/<(\/)? (\w+) (\/)?>/mg, '<$1$2$3>').
        replace(/\s+/mg, ' ').
        replace(/new (\w+)\s*\(\s*\)/mg, 'new $1');

    var actualP = actual.
        replace(/([(){},.:\[\]])/mg, ' $1 ').
        replace(/(\w+)/mg, ' $1 ').
        replace(/<(\/)? (\w+) (\/)?>/mg, '<$1$2$3>').
        replace(/\s+/mg, ' ').
        replace(/new (\w+)\s*\(\s*\)/mg, 'new $1');

    print('expect:\n' + expectP);
    print('actual:\n' + actualP);

    TEST(n, expectP, actualP);

    
    try
    {
        var expectCompile = 'No Error';
        var actualCompile;

        eval(expect);
        try
        {
            eval(actual);
            actualCompile = 'No Error';
        }
        catch(ex1)
        {
            actualCompile = ex1 + '';
        }
        TEST(n, expectCompile, actualCompile);
    }
    catch(ex)
    {
    }
}

if (typeof options == 'function')
{
  options('xml');
}

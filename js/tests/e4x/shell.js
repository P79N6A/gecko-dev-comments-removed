








































gTestsuite = 'e4x';




function reportFailure (section, msg)
{
    msg = inSection(section)+"\n"+msg;
    var lines = msg.split ("\n");
    for (var i=0; i<lines.length; i++)
        print (FAILED + lines[i]);
}

function START(summary)
{
    SUMMARY = summary;
    printStatus(summary);
}

function TEST(section, expected, actual)
{
    var expected_t = typeof expected;
    var actual_t = typeof actual;
    var output = "";
   
    SECTION = section;
    EXPECTED = expected;
    ACTUAL = actual;

    return reportCompare(expected, actual, inSection(section) + SUMMARY);
}

function TEST_XML(section, expected, actual)
{
    var actual_t = typeof actual;
    var expected_t = typeof expected;

    SECTION = section;
    EXPECTED = expected;
    ACTUAL = actual;

    if (actual_t != "xml") {
        
        return TEST(section, new XML(), actual);
    }
 
    if (expected_t == "string") {
        return TEST(section, expected, actual.toXMLString());
    }

    if (expected_t == "number") {
        return TEST(section, String(expected), actual.toXMLString());
    }
 
    throw section + ": Bad TEST_XML usage: type of expected is " +
        expected_t + ", should be number or string";

    
    return false;
}

function SHOULD_THROW(section)
{
    TEST(section, "exception", "no exception");
}

function END()
{
}

if (typeof options == 'function')
{
  options('xml');
}

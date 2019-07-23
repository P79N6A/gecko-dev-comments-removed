








































var FAILED = "FAILED!: ";
var STATUS = "STATUS: ";
var BUGNUMBER = "BUGNUMBER: ";
var VERBOSE = false;
var SECT_PREFIX = 'Section ';
var SECT_SUFFIX = ' of test -';

if (typeof options != 'undefined' &&
    options().indexOf('xml') < 0)
{
    
    options('xml');
}





function expectExitCode(n)
{

    print('--- NOTE: IN THIS TESTCASE, WE EXPECT EXIT CODE ' + n + ' ---');

}




function inSection(x)
{
    return SECT_PREFIX + x + SECT_SUFFIX;
}




function reportFailure (section, msg)
{
    msg = inSection(section)+"\n"+msg;
    var lines = msg.split ("\n");
    for (var i=0; i<lines.length; i++)
        print (FAILED + lines[i]);
}




function printStatus (msg)
{
    msg = msg.toString();
    var lines = msg.split ("\n");
    var l;

    for (var i=0; i<lines.length; i++)
        print (STATUS + lines[i]);

}




function printBugNumber (num)
{
  print (BUGNUMBER + num);
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























function JavaScriptOptions()
{
  this.orig   = {};
  this.orig.strict = this.strict = false;
  this.orig.werror = this.werror = false;

  this.privileges = 'UniversalXPConnect UniversalPreferencesRead ' + 
                    'UniversalPreferencesWrite';

  if (typeof options == 'function')
  {
    
    var optString = options();
    if (optString)
    {
      var optList = optString.split(',');
      for (var iOpt = 0; iOpt < optList.length; iOpt++)
      {
        optName = optList[iOpt];
        this[optName] = true;
      }
    }
  }
  else if (typeof netscape != 'undefined' && 'security' in netscape)
  {
    
    netscape.security.PrivilegeManager.enablePrivilege(this.privileges);

    var preferences = Components.classes['@mozilla.org/preferences-service;1'];
    if (!preferences)
    {
      throw 'JavaScriptOptions: unable to get @mozilla.org/preferences-service;1';
    }

    var prefService = preferences.
      getService(Components.interfaces.nsIPrefService);

    if (!prefService)
    {
      throw 'JavaScriptOptions: unable to get nsIPrefService';
    }

    var pref = prefService.getBranch('');

    if (!pref)
    {
      throw 'JavaScriptOptions: unable to get prefService branch';
    }

    try
    {
      this.orig.strict = this.strict = 
        pref.getBoolPref('javascript.options.strict');
    }
    catch(e)
    {
    }

    try
    {
      this.orig.werror = this.werror = 
        pref.getBoolPref('javascript.options.werror');
    }
    catch(e)
    {
    }
  }
}

JavaScriptOptions.prototype.setOption = 
function (optionName, optionValue)
{
  if (typeof options == 'function')
  {
    
    if (this[optionName] != optionValue)
    {
      options(optionName);
    }
  }
  else if (typeof netscape != 'undefined' && 'security' in netscape)
  {
    
    netscape.security.PrivilegeManager.enablePrivilege(this.privileges);

    var preferences = Components.classes['@mozilla.org/preferences-service;1'];
    if (!preferences)
    {
      throw 'setOption: unable to get @mozilla.org/preferences-service;1';
    }

    var prefService = preferences.
    getService(Components.interfaces.nsIPrefService);

    if (!prefService)
    {
      throw 'setOption: unable to get nsIPrefService';
    }

    var pref = prefService.getBranch('');

    if (!pref)
    {
      throw 'setOption: unable to get prefService branch';
    }

    pref.setBoolPref('javascript.options.' + optionName, optionValue);
  }

  this[optionName] = optionValue;

  return;
}


JavaScriptOptions.prototype.reset = function ()
{
  this.setOption('strict', this.orig.strict);
  this.setOption('werror', this.orig.werror);
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










































var FAILED = "FAILED!: ";
var STATUS = "STATUS: ";
var BUGNUMBER = "BUGNUMBER: ";
var VERBOSE = false;
var SECT_PREFIX = 'Section ';
var SECT_SUFFIX = ' of test -';
var callStack = new Array();





function expectExitCode(n)
{

    print('--- NOTE: IN THIS TESTCASE, WE EXPECT EXIT CODE ' + n + ' ---');

}




function inSection(x)
{

    return SECT_PREFIX + x + SECT_SUFFIX;

}




function inRhino()
{
    return (typeof defineClass == "function");
}




function reportFailure (msg)
{
    var lines = msg.split ("\n");
    var l;
    var funcName = currentFunc();
    var prefix = (funcName) ? "[reported from " + funcName + "] ": "";
    
    for (var i=0; i<lines.length; i++)
        print (FAILED + prefix + lines[i]);

}




function printStatus (msg)
{
    msg = String(msg);
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






function reportCompare (expected, actual, description)
{
    var expected_t = typeof expected;
    var actual_t = typeof actual;
    var output = "";
    
    if ((VERBOSE) && (typeof description != "undefined"))
        printStatus ("Comparing '" + description + "'");

    if (expected_t != actual_t)
        output += "Type mismatch, expected type " + expected_t + 
            ", actual type " + actual_t + "\n";
    else if (VERBOSE)
        printStatus ("Expected type '" + actual_t + "' matched actual " +
                     "type '" + expected_t + "'");

    if (expected != actual)
        output += "Expected value '" + expected + "', Actual value '" + actual +
            "'\n";
    else if (VERBOSE)
        printStatus ("Expected value '" + actual + "' matched actual " +
                     "value '" + expected + "'");

    if (output != "")
    {
        if (typeof description != "undefined")
            reportFailure (description);
        reportFailure (output);   
    }
    else
    {
        print('PASSED! ' + description);
    }
    return (output == ""); 
}





function enterFunc (funcName)
{

    if (!funcName.match(/\(\)$/))
        funcName += "()";

    callStack.push(funcName);

}





function exitFunc (funcName)
{
    var lastFunc = callStack.pop();
    
    if (funcName)
    {
        if (!funcName.match(/\(\)$/))
            funcName += "()";

        if (lastFunc != funcName)
            reportFailure ("Test driver failure, expected to exit function '" +
                           funcName + "' but '" + lastFunc + "' came off " +
                           "the stack");
    }
    
}




function currentFunc()
{
    
    return callStack[callStack.length - 1];
    
}






function BigO(data)
{
  var order = 0;
  var origLength = data.X.length;

  while (data.X.length > 2)
  {
    var lr = new LinearRegression(data);
    if (lr.b > 1e-6)
    {
      
      
      order++;
    }

    if (lr.r > 0.98 || lr.Syx < 1 || lr.b < 1e-6)
    {
      
      
      
      break;
    }
    data = dataDeriv(data);
  }
 
  if (2 == origLength - order)
  {
    order = Number.POSITIVE_INFINITY;
  }
  return order;

  function LinearRegression(data)
    {
      






      var i;

      if (data.X.length != data.Y.length)
      {
        throw 'LinearRegression: data point length mismatch';
      }
      if (data.X.length < 3)
      {
        throw 'LinearRegression: data point length < 2';
      }
      var n = data.X.length;
      var X = data.X;
      var Y = data.Y;

      this.Xavg = 0;
      this.Yavg = 0;

      var SUM_X  = 0;
      var SUM_XY = 0;
      var SUM_XX = 0;
      var SUM_Y  = 0;
      var SUM_YY = 0;

      for (i = 0; i < n; i++)
      {
          SUM_X  += X[i];
          SUM_XY += X[i]*Y[i];
          SUM_XX += X[i]*X[i];
          SUM_Y  += Y[i];
          SUM_YY += Y[i]*Y[i];
      }

      this.b = (n * SUM_XY - SUM_X * SUM_Y)/(n * SUM_XX - SUM_X * SUM_X);
      this.a = (SUM_Y - this.b * SUM_X)/n;

      this.Xavg = SUM_X/n;
      this.Yavg = SUM_Y/n;

      var SUM_Ydiff2 = 0;
      var SUM_Xdiff2 = 0;
      var SUM_XdiffYdiff = 0;

      for (i = 0; i < n; i++)
      {
        var Ydiff = Y[i] - this.Yavg;
        var Xdiff = X[i] - this.Xavg;
        
        SUM_Ydiff2 += Ydiff * Ydiff;
        SUM_Xdiff2 += Xdiff * Xdiff;
        SUM_XdiffYdiff += Xdiff * Ydiff;
      }

      var Syx2 = (SUM_Ydiff2 - Math.pow(SUM_XdiffYdiff/SUM_Xdiff2, 2))/(n - 2);
      var r2   = Math.pow((n*SUM_XY - SUM_X * SUM_Y), 2) /
        ((n*SUM_XX - SUM_X*SUM_X)*(n*SUM_YY-SUM_Y*SUM_Y));

      this.Syx = Math.sqrt(Syx2);
      this.r = Math.sqrt(r2);

    }

  function dataDeriv(data)
    {
      if (data.X.length != data.Y.length)
      {
        throw 'length mismatch';
      }
      var length = data.X.length;

      if (length < 2)
      {
        throw 'length ' + length + ' must be >= 2';
      }
      var X = data.X;
      var Y = data.Y;

      var deriv = {X: [], Y: [] };

      for (var i = 0; i < length - 1; i++)
      {
        deriv.X[i] = (X[i] + X[i+1])/2;
        deriv.Y[i] = (Y[i+1] - Y[i])/(X[i+1] - X[i]);
      }  
      return deriv;
    }

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

function compareSource(expect, actual, summary)
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

    reportCompare(expectP, actualP, summary);

    
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
        reportCompare(expectCompile, actualCompile, 
                      summary + ': compile actual');
    }
    catch(ex)
    {
    }
}

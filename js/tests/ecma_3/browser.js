



















































function htmlesc(str) { 
  if (str == '<') 
    return '&lt;'; 
  if (str == '>') 
    return '&gt;'; 
  if (str == '&') 
    return '&amp;'; 
  return str; 
}

function DocumentWrite(s)
{
  try
  {
    var msgDiv = document.createElement('div');
    msgDiv.innerHTML = s;
    document.body.appendChild(msgDiv);
    msgDiv = null;
  }
  catch(excp)
  {
    document.write(s + '<br>\n');
  }
}

function print() { 
  var s = ''; 
  var a;
  for (var i = 0; i < arguments.length; i++) 
  { 
    a = arguments[i]; 
    s += String(a) + ' '; 
  } 

  if (typeof dump == 'function')
  {
    dump( s + '\n');
  }

  s = s.replace(/[<>&]/g, htmlesc);

  DocumentWrite(s);
}

var testcases = new Array();
var tc = testcases.length;
var bug = '';
var summary = '';
var description = '';
var expected = '';
var actual = '';
var msg = '';

function TestCase(n, d, e, a)
{
  this.path = (typeof gTestPath == 'undefined') ? '' : gTestPath;
  this.name = n;
  this.description = d;
  this.expect = e;
  this.actual = a;
  this.passed = ( e == a );
  this.reason = '';
  this.bugnumber = typeof(bug) != 'undefined' ? bug : '';
  testcases[tc++] = this;
}

var gInReportCompare = false;

var _reportCompare = reportCompare;

reportCompare = function(expected, actual, description)
{
  gInReportCompare = true;

  var testcase = new TestCase(gTestName, description, expected, actual);
  testcase.passed = _reportCompare(expected, actual, description);

  gInReportCompare = false;
};

var _reportFailure = reportFailure;
reportFailure = function (msg, page, line)
{
  var testcase;
  var jsOptions = new JavaScriptOptions();

  jsOptions.setOption('strict', false);
  jsOptions.setOption('werror', false);

  if (gInReportCompare)
  {
    testcase = testcases[tc - 1];
    testcase.passed = false;
  }
  else 
  {
    if (typeof(summary) == "undefined")
    {
      summary = "unknown";
    }
    if (typeof(expect) == "undefined")
    {
      expect = "unknown";
    }
    testcase = new TestCase(gTestName, summary, expect, "error");
    if (document.location.href.indexOf('-n.js') != -1)
    {
      
      testcase.passed = true;
    }
  }

  testcase.reason += msg;

  if (typeof(page) != 'undefined')
  {
    testcase.reason += ' Page: ' + page;
  }
  if (typeof(line) != 'undefined')
  {
    testcase.reason += ' Line: ' + line;
  }
  if (!testcase.passed)
  {
    _reportFailure(msg);
  }

  gDelayTestDriverEnd = false;
  jsTestDriverEnd();

};

function quit()
{
}

window.onerror = reportFailure;

function gc()
{
  
  for (var i = 0; i != 100000; ++i)
  {
    var tmp = new Object();
  }
}

function jsdgc()
{
  try
  {
    
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
    var jsdIDebuggerService = Components.interfaces.jsdIDebuggerService;
    var service = Components.classes['@mozilla.org/js/jsd/debugger-service;1'].
      getService(jsdIDebuggerService);
    service.GC();
  }
  catch(ex)
  {
    print('gc: ' + ex);
  }
}

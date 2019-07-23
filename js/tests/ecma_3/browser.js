



















































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

  optionsPush();

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

  optionsReset();
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

function Preferences(aPrefRoot)
{
  try
  {
    this.orig = {};
    this.privs = 'UniversalXPConnect UniversalPreferencesRead ' + 
      'UniversalPreferencesWrite';

    if (typeof netscape != 'undefined' &&
        'security' in netscape &&
        'PrivilegeManager' in netscape.security &&
        'enablePrivilege' in netscape.security.PrivilegeManager)
    {
      netscape.security.PrivilegeManager.enablePrivilege(this.privs);
    }

    const nsIPrefService = Components.interfaces.nsIPrefService;
    const nsIPrefBranch = Components.interfaces.nsIPrefBranch;
    const nsPrefService_CONTRACTID = "@mozilla.org/preferences-service;1";

    this.prefRoot    = aPrefRoot;
    this.prefService = Components.classes[nsPrefService_CONTRACTID].
      getService(nsIPrefService);
    this.prefBranch = this.prefService.getBranch(aPrefRoot).
      QueryInterface(Components.interfaces.nsIPrefBranch2);
  }
  catch(ex)
  {
  }

}

function Preferences_getPrefRoot() 
{
  try
  {
    if (typeof netscape != 'undefined' &&
        'security' in netscape &&
        'PrivilegeManager' in netscape.security &&
        'enablePrivilege' in netscape.security.PrivilegeManager)
    {
      netscape.security.PrivilegeManager.enablePrivilege(this.privs);
    }

    return this.prefBranch.root; 
  }
  catch(ex)
  {
    return;
  }
}

function Preferences_getPref(aPrefName) 
{
  var value;
  try
  {
    if (typeof netscape != 'undefined' &&
        'security' in netscape &&
        'PrivilegeManager' in netscape.security &&
        'enablePrivilege' in netscape.security.PrivilegeManager)
    {
      netscape.security.PrivilegeManager.enablePrivilege(this.privs);
    }

    try
    {    
      value = this.prefBranch.getBoolPref(aPrefName);
    }
    catch(ex)
    {
      
    }
  }
  catch(ex)
  {
  }
  return value;
}

function Preferences_setPref(aPrefName, aPrefValue) 
{
  try
  {
    if (typeof netscape != 'undefined' &&
        'security' in netscape &&
        'PrivilegeManager' in netscape.security &&
        'enablePrivilege' in netscape.security.PrivilegeManager)
    {
      netscape.security.PrivilegeManager.enablePrivilege(this.privs);
    }

    if (typeof this.orig[aPrefName] == 'undefined')
    {
      this.orig[aPrefName] = this.getPref(aPrefName);
    }

    try
    {
      value = this.prefBranch.setBoolPref(aPrefName, aPrefValue);
    }
    catch(ex)
    {
      
    }
  }
  catch(ex)
  {
  }
}

function Preferences_resetPref(aPrefName) 
{
  try
  {
    if (typeof netscape != 'undefined' &&
        'security' in netscape &&
        'PrivilegeManager' in netscape.security &&
        'enablePrivilege' in netscape.security.PrivilegeManager)
    {
      netscape.security.PrivilegeManager.enablePrivilege(this.privs);
    }

    if (aPrefName in this.orig)
    {
      this.setPref(aPrefName, this.orig[aPrefName]);
    }
  }
  catch(ex)
  {
  }
}

function Preferences_resetAllPrefs() 
{
  try
  {
    var prefName;
    var prefValue;

    if (typeof netscape != 'undefined' &&
        'security' in netscape &&
        'PrivilegeManager' in netscape.security &&
        'enablePrivilege' in netscape.security.PrivilegeManager)
    {
      netscape.security.PrivilegeManager.enablePrivilege(this.privs);
    }

    for (prefName in this.orig)
    {
      this.setPref(prefName, this.orig[prefName]);
    }
  }
  catch(ex)
  {
  }
}

function Preferences_clearPref(aPrefName) 
{
  try
  {
    if (typeof netscape != 'undefined' &&
        'security' in netscape &&
        'PrivilegeManager' in netscape.security &&
        'enablePrivilege' in netscape.security.PrivilegeManager)
    {
      netscape.security.PrivilegeManager.enablePrivilege(this.privs);
    }

    this.prefBranch.clearUserPref(aPrefName);
  }
  catch(ex)
  {
  }
}

Preferences.prototype.getPrefRoot    = Preferences_getPrefRoot;
Preferences.prototype.getPref        = Preferences_getPref;
Preferences.prototype.setPref        = Preferences_setPref;
Preferences.prototype.resetAllPrefs  = Preferences_resetAllPrefs;
Preferences.prototype.resetPref      = Preferences_resetPref;
Preferences.prototype.clearPref      = Preferences_clearPref;

function options(aOptionName) 
{
  
  

  var value = '';
  for (var optionName in options.currvalues)
  {
    value += optionName + ',';
  }
  if (value)
  {
    value = value.substring(0, value.length-1);
  }

  if (aOptionName)
  {
    if (options.currvalues[aOptionName])
    {
      
      delete options.currvalues[aOptionName];
      options.preferences.setPref(aOptionName, false);
    }
    else
    {
      
      options.currvalues[aOptionName] = true;
      options.preferences.setPref(aOptionName, true);
    }
  }

  return value;
}

function optionsInit() {

  
  options.currvalues = {strict:     '', 
                        werror:     '', 
                        atline:     '', 
                        xml:        '',
                        relimit:    '', 
                        anonfunfux: ''
  }

  
  
  options.initvalues  = {};

  
  
  options.stackvalues = [];

  options.preferences = new Preferences('javascript.options.');

  for (var optionName in options.currvalues)
  {
    if (!options.preferences.getPref(optionName))
    {
      delete options.currvalues[optionName];
    }
    else
    {
      options.initvalues[optionName] = '';
    }
  }
}

optionsInit();
optionsClear();

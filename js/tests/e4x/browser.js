


















































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
var BUGSTR = '';
var SUMMARY = '';
var DESCRIPTION = '';
var EXPECTED = '';
var ACTUAL = '';
var MSG = '';
var SECTION = '';

function TestCase(n, d, e, a)
{
  this.path = (typeof gTestPath == 'undefined') ? '' : gTestPath;
  this.name = n;
  this.description = d;
  this.expect = e;
  this.actual = a;
  this.passed = ( e == a );
  this.reason = '';
  this.bugnumber = typeof(BUGSTR) != 'undefined' ? BUGSTR : '';
  testcases[tc++] = this;
}

function reportSuccess(section, expected, actual)
{
  var testcase = new TestCase(gTestName,  SUMMARY + DESCRIPTION + ' Section ' + section, expected, actual);
  testcase.passed = true;
};

function reportError(msg, page, line)
{
  var testcase;

  optionsPush();

  if (typeof SUMMARY == 'undefined')
  {
    SUMMARY = 'Unknown';
  }
  if (typeof SECTION == 'undefined')
  {
    SECTION = 'Unknown';
  }
  if (typeof DESCRIPTION == 'undefined')
  {
    DESCRIPTION = 'Unknown';
  }
  if (typeof EXPECTED == 'undefined')
  {
    EXPECTED = 'Unknown';
  }

  testcase = new TestCase(gTestName, SUMMARY + DESCRIPTION + ' Section ' + SECTION, EXPECTED, "error");

  testcase.passed = false;
  testcase.reason += msg;

  if (typeof(page) != 'undefined')
  {
    testcase.reason += ' Page: ' + page;
  }
  if (typeof(line) != 'undefined')
  {
    testcase.reason += ' Line: ' + line;
  }
  reportFailure(SECTION, msg);

  gDelayTestDriverEnd = false;
  jsTestDriverEnd();

  optionsReset();
};


var _reportFailure = reportFailure;
reportFailure = function (section, msg)
{
  var testcase;

  testcase = new TestCase(gTestName, SUMMARY + DESCRIPTION + ' Section ' + section, EXPECTED, ACTUAL);

  testcase.passed = false;
  testcase.reason += msg;

  _reportFailure(section, msg);

};


var _printBugNumber = printBugNumber;
printBugNumber = function (num)
{
  BUGSTR = BUGNUMBER + num;
  _printBugNumber(num);
}

var _START = START;
START = function (summary)
{
  SUMMARY = summary;
  printStatus(summary);
}

var _TEST = TEST;
TEST = function (section, expected, actual)
{
  SECTION = section;
  EXPECTED = expected;
  ACTUAL = actual;
  if (_TEST(section, expected, actual))
  {
    reportSuccess(section, expected, actual);
  }
}

var _TEST_XML = TEST_XML;
TEST_XML = function (section, expected, actual)
{
  SECTION = section;
  EXPECTED = expected;
  ACTUAL = actual;
  if (_TEST_XML(section, expected, actual))
  {
    reportSuccess(section, expected, actual);
  }
}

function gc()
{
  
  var tmp = Math.PI * 1e500, tmp2;
  for (var i = 0; i != 1 << 15; ++i) 
  {
    tmp2 = tmp * 1.5;
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

function quit()
{
}

window.onerror = reportError;

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
options('xml');


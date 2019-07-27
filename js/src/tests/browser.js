




var gPageCompleted;
var GLOBAL = this + '';


var jstestsTestPassesUnlessItThrows = false;
var jstestsRestoreFunction;
var jstestsOptions;







function testPassesUnlessItThrows() {
  jstestsTestPassesUnlessItThrows = true;
}





function include(file) {
  outputscripttag(file, {language: "type", mimetype: "text/javascript"});
}






function setRestoreFunction(restore) {
  jstestsRestoreFunction = restore;
}

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
  var s = 'TEST-INFO | ';
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

function writeHeaderToLog( string ) {
  string = String(string);

  if (typeof dump == 'function')
  {
    dump( string + '\n');
  }

  string = string.replace(/[<>&]/g, htmlesc);

  DocumentWrite( "<h2>" + string + "</h2>" );
}

function writeFormattedResult( expect, actual, string, passed ) {
  string = String(string);

  if (typeof dump == 'function')
  {
    dump( string + '\n');
  }

  string = string.replace(/[<>&]/g, htmlesc);

  var s = "<tt>"+ string ;
  s += "<b>" ;
  s += ( passed ) ? "<font color=#009900> &nbsp;" + PASSED
    : "<font color=#aa0000>&nbsp;" +  FAILED + expect;

  DocumentWrite( s + "</font></b></tt><br>" );
  return passed;
}

window.onerror = function (msg, page, line)
{
  jstestsTestPassesUnlessItThrows = false;

  
  options = jstestsOptions;

  
  if (typeof jstestsRestoreFunction === "function") {
    jstestsRestoreFunction();
  }

  optionsPush();

  if (typeof DESCRIPTION == 'undefined')
  {
    DESCRIPTION = 'Unknown';
  }
  if (typeof EXPECTED == 'undefined')
  {
    EXPECTED = 'Unknown';
  }

  var testcase = new TestCase("unknown-test-name", DESCRIPTION, EXPECTED, "error");

  if (document.location.href.indexOf('-n.js') != -1)
  {
    
    testcase.passed = true;
  }

  testcase.reason = page + ':' + line + ': ' + msg;

  reportFailure(msg);

  optionsReset();
};

function gc()
{
  try
  {
    SpecialPowers.forceGC();
  }
  catch(ex)
  {
    print('gc: ' + ex);
  }
}

function quit()
{
}

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

  if (aOptionName) {
    if (!(aOptionName in SpecialPowers.Cu)) {
      
      
      
      throw "Unsupported JSContext option '"+ aOptionName +"'";
    }

    if (options.currvalues.hasOwnProperty(aOptionName))
      
      delete options.currvalues[aOptionName];
    else
      
      options.currvalues[aOptionName] = true;

    SpecialPowers.Cu[aOptionName] =
      options.currvalues.hasOwnProperty(aOptionName);
  }

  return value;
}




jstestsOptions = options;

function optionsInit() {

  
  options.currvalues = {
    strict:     true,
    werror:     true,
    strict_mode: true
  };

  
  
  options.initvalues = {};

  
  
  options.stackvalues = [];

  for (var optionName in options.currvalues)
  {
    var propName = optionName;

    if (!(propName in SpecialPowers.Cu))
    {
      throw "options.currvalues is out of sync with Components.utils";
    }
    if (!SpecialPowers.Cu[propName])
    {
      delete options.currvalues[optionName];
    }
    else
    {
      options.initvalues[optionName] = true;
    }
  }
}

function gczeal(z)
{
  SpecialPowers.setGCZeal(z);
}

function jit(on)
{
}

function jsTestDriverBrowserInit()
{

  if (typeof dump != 'function')
  {
    dump = print;
  }

  optionsInit();
  optionsClear();

  if (document.location.search.indexOf('?') != 0)
  {
    
    return;
  }

  var properties = {};
  var fields = document.location.search.slice(1).split(';');
  for (var ifield = 0; ifield < fields.length; ifield++)
  {
    var propertycaptures = /^([^=]+)=(.*)$/.exec(fields[ifield]);
    if (!propertycaptures)
    {
      properties[fields[ifield]] = true;
    }
    else
    {
      properties[propertycaptures[1]] = decodeURIComponent(propertycaptures[2]);
      if (propertycaptures[1] == 'language')
      {
        
        properties.mimetype = fields[ifield+1];
      }
    }
  }

  if (properties.language != 'type')
  {
    try
    {
      properties.version = /javascript([.0-9]+)/.exec(properties.mimetype)[1];
    }
    catch(ex)
    {
    }
  }

  if (!properties.version && navigator.userAgent.indexOf('Gecko/') != -1)
  {
    
    
    
    
    
    
    
    
    
    
    
    if (properties.test.match(/^js1_6/))
    {
      properties.version = '1.6';
    }
    else if (properties.test.match(/^js1_7/))
    {
      properties.version = '1.7';
    }
    else if (properties.test.match(/^js1_8/))
    {
      properties.version = '1.8';
    }
    else if (properties.test.match(/^ecma_6\/LexicalEnvironment/))
    {
      properties.version = '1.8';
    }
    else if (properties.test.match(/^ecma_6\/Class/))
    {
      properties.version = '1.8';
    }
    else if (properties.test.match(/^ecma_6\/extensions/))
    {
      properties.version = '1.8';
    }
  }

  
  
  if (!properties.language)
  {
    properties.language = 'type';
    properties.mimetype = 'text/javascript';
  }

  gTestPath = properties.test;

  if (properties.gczeal)
  {
    gczeal(Number(properties.gczeal));
  }

  








  if (properties.jit  || !document.location.href.match(/jsreftest.html/))
    jit(properties.jit);

  var testpathparts = properties.test.split(/\//);

  if (testpathparts.length < 3)
  {
    
    return;
  }

  document.write('<title>' + properties.test + '<\/title>');

  
  
  document.write('<script></script>');

  
  
  var prepath = "";
  var i = 0;
  for (end = testpathparts.length - 1; i < end; i++) {
    prepath += testpathparts[i] + "/";
    outputscripttag(prepath + "shell.js", properties);
    outputscripttag(prepath + "browser.js", properties);
  }

  
  outputscripttag(prepath + testpathparts[i], properties);

  
  outputscripttag('js-test-driver-end.js', properties);
  return;
}

function outputscripttag(src, properties)
{
  if (!src)
  {
    return;
  }

  var s = '<script src="' +  src + '" charset="utf-8" ';

  if (properties.language != 'type')
  {
    s += 'language="javascript';
    if (properties.version)
    {
      s += properties.version;
    }
  }
  else
  {
    s += 'type="' + properties.mimetype;
    if (properties.version)
    {
      s += ';version=' + properties.version;
    }
  }
  s += '"><\/script>';

  document.write(s);
}

function jsTestDriverEnd()
{
  
  
  
  
  
  
  

  if (gDelayTestDriverEnd)
  {
    return;
  }

  window.onerror = null;

  
  options = jstestsOptions;

  
  if (typeof jstestsRestoreFunction === "function") {
    jstestsRestoreFunction();
  }

  if (jstestsTestPassesUnlessItThrows) {
    var testcase = new TestCase("unknown-test-name", "", true, true);
    print(PASSED);
    jstestsTestPassesUnlessItThrows = false;
  }

  try
  {
    optionsReset();
  }
  catch(ex)
  {
    dump('jsTestDriverEnd ' + ex);
  }

  if (window.opener && window.opener.runNextTest)
  {
    if (window.opener.reportCallBack)
    {
      window.opener.reportCallBack(window.opener.gWindow);
    }
    setTimeout('window.opener.runNextTest()', 250);
  }
  else
  {
    for (var i = 0; i < gTestcases.length; i++)
    {
      gTestcases[i].dump();
    }

    
    document.documentElement.className = '';
    
    gPageCompleted = true;
  }
}


var dlog = (function (s) {});



var gDialogCloser;
var gDialogCloserObserver;

function registerDialogCloser()
{
  gDialogCloser = SpecialPowers.
    Cc['@mozilla.org/embedcomp/window-watcher;1'].
    getService(SpecialPowers.Ci.nsIWindowWatcher);

  gDialogCloserObserver = {observe: dialogCloser_observe};

  gDialogCloser.registerNotification(gDialogCloserObserver);
}

function unregisterDialogCloser()
{
  gczeal(0);

  if (!gDialogCloserObserver || !gDialogCloser)
  {
    return;
  }

  gDialogCloser.unregisterNotification(gDialogCloserObserver);

  gDialogCloserObserver = null;
  gDialogCloser = null;
}



var gDialogCloserSubjects = [];

function dialogCloser_observe(subject, topic, data)
{
  if (subject instanceof ChromeWindow && topic == 'domwindowopened' )
  {
    gDialogCloserSubjects.push(subject);
    
    subject.setTimeout(closeDialog, 0);
  }
}

function closeDialog()
{
  var subject;

  while ( (subject = gDialogCloserSubjects.pop()) != null)
  {
    if (subject.document instanceof XULDocument &&
        subject.document.documentURI == 'chrome://global/content/commonDialog.xul')
    {
      subject.close();
    }
    else
    {
      
      subject.close();
    }
  }
}

function newGlobal() {
  var iframe = document.createElement("iframe");
  document.documentElement.appendChild(iframe);
  var win = iframe.contentWindow;
  iframe.remove();
  
  win.evaluate = win.eval;
  return win;
}

registerDialogCloser();
window.addEventListener('unload', unregisterDialogCloser, true);

jsTestDriverBrowserInit();

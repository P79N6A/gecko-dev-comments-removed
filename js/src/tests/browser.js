



var gPageCompleted;
var GLOBAL = this + '';

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
    : "<font color=#aa0000>&nbsp;" +  FAILED + expect + "</tt>";

  DocumentWrite( s + "</font></b></tt><br>" );
  return passed;
}

window.onerror = function (msg, page, line)
{
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
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
    Components.utils.forceGC();
  }
  catch(ex)
  {
    print('gc: ' + ex);
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
    print('jsdgc: ' + ex);
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

  if (aOptionName === 'moar_xml')
    aOptionName = 'xml';

  if (aOptionName && aOptionName !== 'allow_xml') {
    netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
    if (!(aOptionName in Components.utils)) {

      
      
      
      throw "Unsupported JSContext option '"+ aOptionName +"'";
    }

    if (options.currvalues.hasOwnProperty(aOptionName))
      
      delete options.currvalues[aOptionName];
    else
      
      options.currvalues[aOptionName] = true;


    Components.utils[aOptionName] =
      options.currvalues.hasOwnProperty(aOptionName);
  }  

  return value;
}

function optionsInit() {

  
  options.currvalues = {
    strict:     true,
    werror:     true,
    atline:     true,
    moar_xml:   true,
    methodjit:  true,
    methodjit_always: true,
    strict_mode: true
  };

  
  
  options.initvalues = {};

  
  
  options.stackvalues = [];

  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
  for (var optionName in options.currvalues)
  {
    var propName = optionName;
    if (optionName === "moar_xml")
      propName = "xml";


    if (!(propName in Components.utils))
    {
      throw "options.currvalues is out of sync with Components.utils";
    }

    if (!Components.utils[propName])
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
  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');
  Components.utils.setGCZeal(z);
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
  var suitepath = testpathparts.slice(0,testpathparts.length-2).join('/');
  var subsuite = testpathparts[testpathparts.length - 2];
  var test     = testpathparts[testpathparts.length - 1];

  document.write('<title>' + suitepath + '/' + subsuite + '/' + test + '<\/title>');

  
  
  document.write('<script></script>');

  outputscripttag(suitepath + '/shell.js', properties);
  outputscripttag(suitepath + '/browser.js', properties);
  outputscripttag(suitepath + '/' + subsuite + '/shell.js', properties);
  outputscripttag(suitepath + '/' + subsuite + '/browser.js', properties);
  outputscripttag(suitepath + '/' + subsuite + '/' + test, properties,
  	properties.e4x || /e4x\//.test(properties.test));
  outputscripttag('js-test-driver-end.js', properties);
  return;
}

function outputscripttag(src, properties, e4x)
{
  if (!src)
  {
    return;
  }

  if (e4x)
  {
    
    properties.language = 'type';
  }

  var s = '<script src="' +  src + '" ';

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
    if (e4x)
    {
      s += ';e4x=1';
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
  netscape.security.PrivilegeManager.enablePrivilege('UniversalXPConnect');

  gDialogCloser = Components.
    classes['@mozilla.org/embedcomp/window-watcher;1'].
    getService(Components.interfaces.nsIWindowWatcher);

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

registerDialogCloser();
window.addEventListener('unload', unregisterDialogCloser, true);

jsTestDriverBrowserInit();

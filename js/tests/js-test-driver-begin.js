
var gVersion = 150;
var gTestName = '';
var gTestPath = '';
var gDelayTestDriverEnd = false;

function init()
{
  if (document.location.search.indexOf('?') != 0)
  {
    
    return;
  }

  var re = /test=([^;]+);language=(language|type);([a-zA-Z0-9.=;\/]+)/;
  var matches = re.exec(document.location.search);

  
  var testpath  = matches[1];
  var attribute = matches[2];
  var value     = matches[3];

  if (testpath)
  {
    gTestPath = testpath;
  }

  var ise4x = /e4x\//.test(testpath);

  if (value.indexOf('1.1') != -1)
  {
    gVersion = 110;
  }
  else if (value.indexOf('1.2') != -1)
  {
    gVersion = 120;
  }
  else if (value.indexOf('1.3') != -1)
  {
    gVersion = 130;
  }
  else if (value.indexOf('1.4') != -1)
  {
    gVersion = 140;
  }
  else if(value.indexOf('1.5') != -1)
  {
    gVersion = 150;
  }
  else if(value.indexOf('1.6') != -1)
  {
    gVersion = 160;
  }
  else if(value.indexOf('1.7') != -1)
  {
    gVersion = 170;
  }

  var testpathparts = testpath.split(/\//);

  if (testpathparts.length < 3)
  {
    
    return;
  }
  var suitepath = testpathparts.slice(0,testpathparts.length-2).join('/');
  var subsuite  = testpathparts[testpathparts.length - 2];
  var test      = testpathparts[testpathparts.length - 1];

  gTestName = test;

  outputscripttag(suitepath + '/shell.js', attribute, value, ise4x);
  outputscripttag(suitepath + '/browser.js', attribute, value, ise4x);
  outputscripttag(suitepath + '/' + subsuite + '/shell.js', attribute, value, 
                  ise4x);
  outputscripttag(suitepath + '/' + subsuite + '/browser.js', attribute, value,
                  ise4x);
  outputscripttag(suitepath + '/' + subsuite + '/' + test, attribute, value, 
                  ise4x);

  document.write('<title>' + suitepath + '/' + subsuite + '/' + test + 
                 '<\/title>');
  return;
}

function outputscripttag(src, attribute, value, ise4x)
{
  if (!src)
  {
    return;
  }

  var s = '<script src="' +  src + '" ';

  if (ise4x)
  {
    if (attribute == 'type')
    {
      value += ';e4x=1 ';
    }
    else
    {
      s += ' type="text/javascript';
      if (gVersion != 150)
      {
        s += ';version=' + gVersion/100;
      }
      s += ';e4x=1" ';
    }
  }

  s +=  attribute + '="' + value + '"><\/script>';

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
    gPageCompleted = true;
  }
}

init();


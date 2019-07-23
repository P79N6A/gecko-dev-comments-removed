









































var gCheckInterval = 1000;
var gCurrentTestId;
var gReport;

function userOnStart()
{
  dlog('userOnStart');
  registerDialogCloser();
}

function userOnBeforePage()
{
  dlog('userOnBeforePage');
  gCurrentTestId = /test=(.*);language/.exec(gSpider.mCurrentUrl.mUrl)[1];
  cdump('JavaScriptTest: Begin Test ' + gCurrentTestId);
  cdump('JavaScriptTest: Begin Execution ' + gCurrentTestId);
}

function userOnAfterPage()
{
  dlog('userOnAfterPage');
  cdump('JavaScriptTest: End Execution ' + gCurrentTestId);
  checkTestCompleted();
}

function userOnStop()
{
  
  cdump('JavaScriptTest: End Test ' + gCurrentTestId);
  closeDialog();
  unregisterDialogCloser();
}

function checkTestCompleted()
{
  dlog('checkTestCompleted()');

  var win = gSpider.mDocument.defaultView;
  if (win.wrappedJSObject)
  {
    win = win.wrappedJSObject;
  }
  if (win.gPageCompleted)
  {
    dlog('Page Completed');

    cdump('JavaScriptTest: Begin Summary ' + gCurrentTestId);

    gPageCompleted = true;

    var gTestcases = win.gTestcases;
    if (typeof gTestcases == 'undefined')
    {
      cdump('JavaScriptTest: ' + gCurrentTestId +
            ' gTestcases array not defined. Possible test failure.');
    }
    else if (gTestcases.length == 0)
    {
      cdump('JavaScriptTest: ' + gCurrentTestId +
            ' gTestcases array is empty. Tests not run.');
    }
    else
    {
    }
    cdump('JavaScriptTest: End Summary ' + gCurrentTestId);

    
    
    collectGarbage();
  }
  else
  {
    dlog('page not completed, recheck');
    setTimeout(checkTestCompleted, gCheckInterval);
  }
 
}

gConsoleListener.onConsoleMessage =
  function userOnConsoleMessage(s)
{
  dump(s);
};

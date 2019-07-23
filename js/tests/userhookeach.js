









































var gCheckInterval = 1000;
var gCurrentTestId;
var gReport;
var gCurrentTestStart;
var gCurrentTestStop;
var gCurrentTestValid;
var gPageStart;
var gPageStop;

function userOnStart()
{
  dlog('userOnStart');
  cdump('JavaScriptTest: Begin Run');
  registerDialogCloser();
}

function userOnBeforePage()
{
  dlog('userOnBeforePage');
  gPageStart = new Date();

  try
  {
    gCurrentTestId = /test=(.*);language/.exec(gSpider.mCurrentUrl.mUrl)[1];
    gCurrentTestValid = true;
    cdump('JavaScriptTest: Begin Test ' + gCurrentTestId);
    gCurrentTestStart = new Date();
  }
  catch(ex)
  {
    cdump('userOnBeforePage: ' + ex);
    gCurrentTestValid = false;
    gPageCompleted = true;
  }
}

function userOnAfterPage()
{
  dlog('userOnAfterPage');
  gPageStop = new Date();

  cdump(gSpider.mCurrentUrl.mUrl + ': PAGE STATUS: NORMAL (' + ((gPageStop - gPageStart)/1000).toFixed(0) + ' seconds)');
  checkTestCompleted();
}

function userOnStop()
{
  
  cdump('JavaScriptTest: End Run');
  closeDialog();
  unregisterDialogCloser();
}

function userOnPageTimeout()
{
  gPageStop = new Date();
  cdump(gSpider.mCurrentUrl.mUrl + ': PAGE STATUS: TIMED OUT (' + ((gPageStop - gPageStart)/1000).toFixed(0) + ' seconds)');
  cdump('JavaScriptTest: End Test ' + gCurrentTestId);
}

function checkTestCompleted()
{
  dlog('checkTestCompleted()');

  var win = gSpider.mDocument.defaultView;
  if (win.wrappedJSObject)
  {
    win = win.wrappedJSObject;
  }

  if (!gCurrentTestValid)
  {
    gPageCompleted = true;
  }
  else if (win.gPageCompleted)
  {
    gCurrentTestStop = new Date();
    
    collectGarbage();

    dlog('Page Completed');

    var gTestcases = win.gTestcases;
    if (typeof gTestcases == 'undefined')
    {
      cdump('JavaScriptTest: ' + gCurrentTestId +
            ' gTestcases array not defined. Possible test failure.');
      throw 'gTestcases array not defined. Possible test failure.';
    }
    else if (gTestcases.length == 0)
    {
      cdump('JavaScriptTest: ' + gCurrentTestId +
            ' gTestcases array is empty. Tests not run.');
      new win.TestCase(win.gTestFile, win.summary, 'Unknown', 'gTestcases array is empty. Tests not run..');
    }
    else
    {
    }
    cdump('JavaScriptTest: ' + gCurrentTestId + ' Elapsed time ' + ((gCurrentTestStop - gCurrentTestStart)/1000).toFixed(2) + ' seconds');
    cdump('JavaScriptTest: End Test ' + gCurrentTestId);

    gPageCompleted = true;
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

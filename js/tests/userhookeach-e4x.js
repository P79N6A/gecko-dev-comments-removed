










































var gCheckInterval = 1000;



function userOnStart()
{
  registerDialogCloser();
}

function userOnBeforePage()
{
  dlog('userOnBeforePage');
}

var gReport;

function userOnAfterPage()
{
  dlog('userOnAfterPage');
  checkTestCompleted();
}

function userOnStop()
{
  
  closeDialog();
  unregisterDialogCloser();
}

function checkTestCompleted()
{
  var win = gSpider.mDocument.defaultView;
  if (win.wrappedJSObject)
  {
    win = win.wrappedJSObject;
  }
  if (win.gPageCompleted)
  {
    unregisterDialogCloser();
    gPageCompleted = true;

    var gTestcases = win.gTestcases;
    if (typeof gTestcases == 'undefined')
    {
      return;
    }

    for (var i = 0; i < gTestcases.length; i++)
    {
      var testcase = gTestcases[i];
      cdump('test: '    + testcase.path + ' ' +
            'bug: '         + testcase.bugnumber + ' ' +
            'result: ' + (testcase.passed ? 'PASSED':'FAILED') + ' ' +
            'type: browser ' +
            'description: ' + testcase.description + ' ' +
            'expected: '    + testcase.expect + ' ' +
            'actual: '      + testcase.actual + ' ' +
            'reason: '      + testcase.reason);
    }
    
    
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

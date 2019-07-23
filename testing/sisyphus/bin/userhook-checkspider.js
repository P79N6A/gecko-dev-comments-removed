









































function userOnStart()
{
}

function userOnBeforePage()
{
}

function userOnAfterPage()
{
  var win = gSpider.mDocument.defaultView;
  if (win.wrappedJSObject)
  {
    win = win.wrappedJSObject;
  }

  dumpObject('navigator', win.navigator);

  for (var i = 0; i < win.navigator.mimeTypes.length; i++)
  {
    dumpObject('navigator.mimeTypes[' + i + ']', win.navigator.mimeTypes[i]);
  }

  for (var i = 0; i < win.navigator.plugins.length; i++)
  {
    dumpObject('navigator.plugins[' + i + ']', win.navigator.plugins[i]);
  }

  gPageCompleted = true;
}

function dumpObject(name, object)
{
  for (var p in object)
  {
    if (/(string|number)/.test(typeof object[p]))
    {
      cdump(name + '.' + p + ':' + object[p]);
    }
  }

}

function userOnStop()
{
}


gConsoleListener.onConsoleMessage = 
function userOnConsoleMessage(s)
{
  dump(s);
};

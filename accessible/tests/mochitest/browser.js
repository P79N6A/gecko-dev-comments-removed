


function openBrowserWindow(aFunc, aURL, aRect)
{
  gBrowserContext.testFunc = aFunc;
  gBrowserContext.startURL = aURL;
  gBrowserContext.browserRect = aRect;

  addLoadEvent(openBrowserWindowIntl);
}




function closeBrowserWindow()
{
  gBrowserContext.browserWnd.close();
}




function browserWindow()
{
  return gBrowserContext.browserWnd;
}




function browserDocument()
{
  return browserWindow().document;
}




function tabBrowser()
{
  return browserWindow().gBrowser;
}




function currentBrowser()
{
  return tabBrowser().selectedBrowser;
}




function currentTabDocument()
{
  return currentBrowser().contentDocument;
}




function currentTabWindow()
{
  return currentTabDocument().defaultView;
}




function browserAt(aIndex)
{
  return tabBrowser().getBrowserAtIndex(aIndex);
}




function tabDocumentAt(aIndex)
{
  return browserAt(aIndex).contentDocument;
}




function urlbarInput()
{
  return browserWindow().document.getElementById("urlbar").inputField;
}




function reloadButton()
{
  return browserWindow().document.getElementById("urlbar-reload-button");
}




Components.utils.import("resource://gre/modules/Services.jsm");

var gBrowserContext =
{
  browserWnd: null,
  testFunc: null,
  startURL: ""
};

function openBrowserWindowIntl()
{
  var params = "chrome,all,dialog=no";
  var rect = gBrowserContext.browserRect;
  if (rect) {
    if ("left" in rect)
      params += ",left=" + rect.left;
    if ("top" in rect)
      params += ",top=" + rect.top;
    if ("width" in rect)
      params += ",width=" + rect.width;
    if ("height" in rect)
      params += ",height=" + rect.height;
  }

  gBrowserContext.browserWnd =
    window.openDialog(Services.prefs.getCharPref("browser.chromeURL"),
                      "_blank", params,
                      gBrowserContext.startURL);

  addA11yLoadEvent(startBrowserTests, browserWindow());
}

function startBrowserTests()
{
  if (gBrowserContext.startURL) 
    addA11yLoadEvent(gBrowserContext.testFunc, currentBrowser().contentWindow);
  else
    gBrowserContext.testFunc();
}

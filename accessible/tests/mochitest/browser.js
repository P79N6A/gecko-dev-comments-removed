


function openBrowserWindow(aFunc, aURL)
{
  gBrowserContext.testFunc = aFunc;
  gBrowserContext.startURL = aURL;

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
  gBrowserContext.browserWnd =
    window.openDialog(Services.prefs.getCharPref("browser.chromeURL"),
                      "_blank", "chrome,all,dialog=no",
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

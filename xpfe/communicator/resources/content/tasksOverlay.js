





































const nsIDOMWindowInternal = Components.interfaces.nsIDOMWindowInternal;
const nsIWindowMediator = Components.interfaces.nsIWindowMediator;
const nsIWindowDataSource = Components.interfaces.nsIWindowDataSource;

function toNavigator()
{
  if (!CycleWindow("navigator:browser"))
    OpenBrowserWindow();
}

function toDownloadManager()
{
  var dlmgr = Components.classes['@mozilla.org/download-manager;1'].getService();
  dlmgr = dlmgr.QueryInterface(Components.interfaces.nsIDownloadManager);

  var windowMediator = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
  windowMediator = windowMediator.QueryInterface(nsIWindowMediator);

  var dlmgrWindow = windowMediator.getMostRecentWindow("Download:Manager");
  if (dlmgrWindow) {
    dlmgrWindow.focus();
  }
  else {
    dlmgr.open(window, null);
  }
}
  
function toJavaScriptConsole()
{
    toOpenWindowByType("global:console", "chrome://global/content/console.xul");
}

function javaItemEnabling()
{
    var element = document.getElementById("java");
    if (navigator.javaEnabled())
      element.removeAttribute("disabled");
    else
      element.setAttribute("disabled", "true");
}
            
function toJavaConsole()
{
    var jvmMgr = Components.classes['@mozilla.org/oji/jvm-mgr;1']
                            .getService(Components.interfaces.nsIJVMManager)
    jvmMgr.showJavaConsole();
}

function toOpenWindow( aWindow )
{
  try {
    
    aWindow.document.commandDispatcher.focusedWindow.focus();
  } catch (e) {
    
    aWindow.focus();
  }
}

function toOpenWindowByType( inType, uri )
{
  
  if (uri in window)
    return;

  var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService(nsIWindowMediator);

  var topWindow = windowManager.getMostRecentWindow( inType );

  if ( topWindow )
    toOpenWindow( topWindow );
  else
  {
    function newWindowLoaded(event) {
      delete window[uri];
    }
    window[uri] = window.openDialog(uri, "", "all,dialog=no");
    window[uri].addEventListener("load", newWindowLoaded, false);
  }
}


function OpenBrowserWindow()
{
  var charsetArg = new String();
  var handler = Components.classes['@mozilla.org/commandlinehandler/general-startup;1?type=browser'];
  handler = handler.getService();
  handler = handler.QueryInterface(Components.interfaces.nsICmdLineHandler);
  var url = handler.chromeUrlForTask;
  var wintype = document.documentElement.getAttribute('windowtype');
  var windowMediator = Components.classes["@mozilla.org/appshell/window-mediator;1"].getService(Components.interfaces.nsIWindowMediator);
  var browserWin = windowMediator.getMostRecentWindow("navigator:browser");
 
  
  
  var startpage = browserWin ? null : handler.defaultArgs;

  
  
  
  if (window && (wintype == "navigator:browser") && window.content && window.content.document)
  {
    var DocCharset = window.content.document.characterSet;
    charsetArg = "charset="+DocCharset;

    
    window.openDialog(url, "_blank", "chrome,all,dialog=no", startpage, charsetArg);
  }
  else 
  {
    window.openDialog(url, "_blank", "chrome,all,dialog=no", startpage);
  }
}

function CycleWindow( aType )
{
  var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
  var windowManagerInterface = windowManager.QueryInterface(nsIWindowMediator);

  var topWindowOfType = windowManagerInterface.getMostRecentWindow( aType );
  var topWindow = windowManagerInterface.getMostRecentWindow( null );

  if ( topWindowOfType == null )
    return null;

  if ( topWindowOfType != topWindow ) {
    toOpenWindow(topWindowOfType);
    return topWindowOfType;
  }

  var enumerator = windowManagerInterface.getEnumerator( aType );
  var firstWindow = enumerator.getNext().QueryInterface(nsIDOMWindowInternal);
  var iWindow = firstWindow;
  while (iWindow != topWindow && enumerator.hasMoreElements())
    iWindow = enumerator.getNext().QueryInterface(nsIDOMWindowInternal);

  if (enumerator.hasMoreElements()) {
    iWindow = enumerator.getNext().QueryInterface(nsIDOMWindowInternal);
    toOpenWindow(iWindow);
    return iWindow;
  }

  if (firstWindow == topWindow) 
    return null;

  toOpenWindow(firstWindow);
  return firstWindow;
}

function ShowWindowFromResource( node )
{
	var windowManagerDS = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService(nsIWindowDataSource);
    
    var desiredWindow = null;
    var url = node.getAttribute('id');
	desiredWindow = windowManagerDS.getWindowForResource( url );
	if ( desiredWindow )
	{
		toOpenWindow(desiredWindow);
	}
}

function OpenTaskURL( inURL )
{
	
	window.open( inURL );
}

function ShowUpdateFromResource( node )
{
	var url = node.getAttribute('url');
        
        
	OpenTaskURL( "http://www.mozilla.org/binaries.html");
}

function checkFocusedWindow()
{
  var windowManagerDS = Components.classes['@mozilla.org/rdf/datasource;1?name=window-mediator'].getService(nsIWindowDataSource);

  var sep = document.getElementById("sep-window-list");
  
  while ((sep = sep.nextSibling)) {
    var url = sep.getAttribute('id');
    var win = windowManagerDS.getWindowForResource(url);
    if (win == window) {
      sep.setAttribute("checked", "true");
      break;
    }
  }
}

function toProfileManager()
{
  const wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                         .getService(Components.interfaces.nsIWindowMediator);
  var promgrWin = wm.getMostRecentWindow( "mozilla:profileSelection" );
  if (promgrWin) {
    promgrWin.focus();
  } else {
    var params = Components.classes["@mozilla.org/embedcomp/dialogparam;1"]
                 .createInstance(Components.interfaces.nsIDialogParamBlock);
  
    params.SetNumberStrings(1);
    params.SetString(0, "menu");
    window.openDialog("chrome://communicator/content/profile/profileSelection.xul",
                "",
                "centerscreen,chrome,titlebar",
                params);
  }
  
  
}


function ZoomCurrentWindow()
{
  if (window.windowState == STATE_NORMAL)
    window.maximize();
  else
    window.restore();
}








































function initEditorContextMenuItems(aEvent)
{
  var shouldShowEditPage = !gContextMenu.onImage && !gContextMenu.onLink && !gContextMenu.onTextInput && !gContextMenu.inDirList;
  gContextMenu.showItem( "context-editpage", shouldShowEditPage );

  var shouldShowEditLink = gContextMenu.onSaveableLink; 
  gContextMenu.showItem( "context-editlink", shouldShowEditLink );

  
  gContextMenu.showItem("context-sep-apps", gContextMenu.shouldShowSeparator("context-sep-apps"));
}
  
function initEditorContextMenuListener(aEvent)
{
  var popup = document.getElementById("contentAreaContextMenu");
  if (popup)
    popup.addEventListener("popupshowing", initEditorContextMenuItems, false);
}

addEventListener("load", initEditorContextMenuListener, false);

function editDocument(aDocument)      
{
  if (!aDocument)
    aDocument = window.content.document;

  editPage(aDocument.URL); 
}

function editPageOrFrame()
{
  var focusedWindow = document.commandDispatcher.focusedWindow;

  
  
  editPage(getContentFrameURI(focusedWindow));
}




function editPage(url)
{
  
  url = url.replace(/^view-source:/, "").replace(/#.*/, "");

  
  

  var wintype = document.documentElement.getAttribute('windowtype');
  var charsetArg;

  if (wintype == "navigator:browser" && content.document)
    charsetArg = "charset=" + content.document.characterSet;

  try {
    var uri = createURI(url, null, null);

    var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1'].getService();
    var windowManagerInterface = windowManager.QueryInterface( Components.interfaces.nsIWindowMediator);
    var enumerator = windowManagerInterface.getEnumerator( "composer:html" );
    var emptyWindow;
    while ( enumerator.hasMoreElements() )
    {
      var win = enumerator.getNext().QueryInterface(Components.interfaces.nsIDOMWindowInternal);
      if ( win && win.IsWebComposer())
      {
        if (CheckOpenWindowForURIMatch(uri, win))
        {
          
          win.focus();
          return;
        }
        else if (!emptyWindow && win.PageIsEmptyAndUntouched())
        {
          emptyWindow = win;
        }
      }
    }

    if (emptyWindow)
    {
      
      if (emptyWindow.IsInHTMLSourceMode())
        emptyWindow.SetEditMode(emptyWindow.PreviousNonSourceDisplayMode);
      emptyWindow.EditorLoadUrl(url);
      emptyWindow.focus();
      emptyWindow.SetSaveAndPublishUI(url);
      return;
    }

    
    openDialog("chrome://editor/content", "_blank", "chrome,all,dialog=no", url, charsetArg);

  } catch(e) {}
}

function createURI(urlstring)
{
  try {
    var ioserv = Components.classes["@mozilla.org/network/io-service;1"]
               .getService(Components.interfaces.nsIIOService);
    return ioserv.newURI(urlstring, null, null);
  } catch (e) {}

  return null;
}

function CheckOpenWindowForURIMatch(uri, win)
{
  try {
    var contentWindow = win.content;  
    var contentDoc = contentWindow.document;
    var htmlDoc = contentDoc.QueryInterface(Components.interfaces.nsIDOMHTMLDocument);
    var winuri = createURI(htmlDoc.URL);
    return winuri.equals(uri);
  } catch (e) {}
  
  return false;
}

function NewEditorFromTemplate()
{
  
}

function NewEditorFromDraft()
{
  
}

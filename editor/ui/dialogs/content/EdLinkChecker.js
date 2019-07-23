







































var gNumLinksToCheck = 0;     
var gLinksBeingChecked = [];  
var gURIRefObjects = [];      
var gNumLinksCalledBack = 0;
var gStartedAllChecks = false;
var gLinkCheckTimerID = 0;


var gRequestObserver =
{
  
  onStartRequest: function(request, ctxt) { },

  
  onStopRequest: function(request, ctxt, status)
  {
    var linkChecker = request.QueryInterface(Components.interfaces.nsIURIChecker);
    if (linkChecker)
    {
      gNumLinksCalledBack++;
      linkChecker.status = status;
      for (var i = 0; i < gNumLinksCalledBack; i++)
      {
        if (linkChecker == gLinksBeingChecked[i])
          gLinksBeingChecked[i].status = status;
      }

      if (gStartedAllChecks && gNumLinksCalledBack >= gNumLinksToCheck)
      {
        clearTimeout(gLinkCheckTimerID);
        LinkCheckTimeOut();
      }
    }
  }
}

function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    window.close();
    return;
  }

  
  var objects;
  try {
    objects = editor.getLinkedObjects();
  } catch (e) {}

  if (!objects || objects.Count() == 0)
  {
    AlertWithTitle(GetString("Alert"), GetString("NoLinksToCheck"));
    window.close();
    return;
  }

  gDialog.LinksList = document.getElementById("LinksList");

  
  SetWindowLocation();


  
  for (var i = 0; i < objects.Count(); i++)
  {
    var refobj = objects.GetElementAt(gNumLinksToCheck).QueryInterface(Components.interfaces.nsIURIRefObject);
    
    if (refobj)
    {
      try {
        var uri;
        while ((uri = refobj.GetNextURI()))
        {
          
          
          gURIRefObjects[gNumLinksToCheck] = refobj;

          
          gLinksBeingChecked[gNumLinksToCheck]
            = Components.classes["@mozilla.org/network/urichecker;1"]
                .createInstance()
                  .QueryInterface(Components.interfaces.nsIURIChecker);
          
          gLinksBeingChecked[gNumLinksToCheck].init(GetIOService().newURI(uri, null, null));
          gLinksBeingChecked[gNumLinksToCheck].asyncCheck(gRequestObserver, null);

          
          var linkChecker = gLinksBeingChecked[gNumLinksToCheck].QueryInterface(Components.interfaces.nsIURIChecker);
          SetItemStatus(linkChecker.name, "busy");
dump(" *** Linkcount = "+gNumLinksToCheck+"\n");
          gNumLinksToCheck++;

        };
      } catch (e) { dump (" *** EXCEPTION\n");}
    }
  }
  
  gStartedAllChecks = true;

  
  gLinkCheckTimerID = setTimeout("LinkCheckTimeOut()", 5000);
  window.sizeToContent();
}

function LinkCheckTimeOut()
{
  
  if (gNumLinksToCheck <= 0)
    return;
  gLinkCheckTimerID = 0;

  gNumLinksToCheck = 0;
  gStartedAllChecks = false;
  for (var i=0; i < gLinksBeingChecked.length; i++)
  {
    var linkChecker = gLinksBeingChecked[i].QueryInterface(Components.interfaces.nsIURIChecker);
    
    
    
    
    switch (linkChecker.status)
    {
      case 0:           
        SetItemStatus(linkChecker.name, "done");
        break;
      case 0x804b0001:  
        dump(">> " + linkChecker.name + " is broken\n");
      case 0x804b0002:   

      default:

        SetItemStatus(linkChecker.name, "failed");
        break;
    }
  }
}




function SetItemStatus(url, status)
{
  if (!url)
    return false;

  if (!status)
    status = "busy";

  
  
  var listitems = document.getElementsByTagName("listitem");
  if (listitems)
  {
    for (var i=0; i < listitems.length; i++)
    {
      if (listitems[i].getAttribute("label") == url)
      {
        listitems[i].setAttribute("progress", status);
        return true;
      }
    }
  }

  
  var listitem = document.createElementNS(XUL_NS, "listitem");
  if (listitem)
  {
    listitem.setAttribute("class", "listitem-iconic progressitem");
    
    listitem.setAttribute("progress", status);
    listitem.setAttribute("label", url);
    gDialog.LinksList.appendChild(listitem);
  }
  return false;
}

function onAccept()
{
  SaveWindowLocation();
  return true; 
}

function onCancelLinkChecker()
{
  if (gLinkCheckTimerID)
    clearTimeout(gLinkCheckTimerID);




  return onCancel();
}

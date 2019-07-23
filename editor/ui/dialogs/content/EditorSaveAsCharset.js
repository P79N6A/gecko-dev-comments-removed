








































var gCharset="";
var gTitleWasEdited = false;
var gCharsetWasChanged = false;
var gInsertNewContentType = false;
var gContenttypeElement;
var gInitDone = false;



function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    window.close();
    return;
  }

  var observerService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  observerService.notifyObservers(null, "charsetmenu-selected", "other");

  gDialog.TitleInput    = document.getElementById("TitleInput");
  gDialog.charsetTree   = document.getElementById('CharsetTree'); 
  gDialog.exportToText  = document.getElementById('ExportToText');

  gContenttypeElement = GetHTTPEquivMetaElement("content-type");
  if (!gContenttypeElement && (editor.contentsMIMEType != 'text/plain')) 
  {
    gContenttypeElement = CreateHTTPEquivMetaElement("content-type");
    if (!gContenttypeElement ) 
	{
      window.close();
      return;
    }
    gInsertNewContentType = true;
  }

  try {
    gCharset = editor.documentCharacterSet;
  } catch (e) {}

  InitDialog();

  
  document.getElementById("EnterTitleLabel").setAttribute("value",GetString("NeedDocTitle"));
  
  var helpTextParent = document.getElementById("TitleHelp");
  var helpText = document.createTextNode(GetString("DocTitleHelp"));
  if (helpTextParent)
    helpTextParent.appendChild(helpText);
  
  
  SetTextboxFocus(gDialog.TitleInput);
  
  gInitDone = true;
  
  SetWindowLocation();
}

  
function InitDialog() 
{
  gDialog.TitleInput.value = GetDocumentTitle();

  var RDF = Components.classes["@mozilla.org/rdf/rdf-service;1"].getService(Components.interfaces.nsIRDFService);
  var tree = gDialog.charsetTree;
  var index = tree.view.getIndexOfResource(RDF.GetResource(gCharset));
  if (index >= 0) {
    tree.view.selection.select(index);
    tree.treeBoxObject.ensureRowIsVisible(index);
  }
}


function onAccept()
{
  var editor = GetCurrentEditor();
  editor.beginTransaction();

  if(gCharsetWasChanged) 
  {
     try {
       SetMetaElementContent(gContenttypeElement, "text/html; charset=" + gCharset, gInsertNewContentType, true);     
      editor.documentCharacterSet = gCharset;
    } catch (e) {}
  }

  editor.endTransaction();

  if(gTitleWasEdited) 
    SetDocumentTitle(TrimString(gDialog.TitleInput.value));

  window.opener.ok = true;
  window.opener.exportToText = gDialog.exportToText.checked;
  SaveWindowLocation();
  return true;
}


function readRDFString(aDS,aRes,aProp) 
{
  var n = aDS.GetTarget(aRes, aProp, true);
  if (n)
    return n.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
  else
    return "";
}

      
function SelectCharset()
{
  if(gInitDone) 
  {
    try 
	{
      gCharset = gDialog.charsetTree.builderView.getResourceAtIndex(gDialog.charsetTree.currentIndex).Value;
      if (gCharset)
        gCharsetWasChanged = true;
    }
    catch(e) {}
  }
}


function TitleChanged()
{
  gTitleWasEdited = true; 
}

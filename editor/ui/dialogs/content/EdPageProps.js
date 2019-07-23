





































var gNewTitle = "";
var gAuthor = "";
var gDescription = "";
var gAuthorElement;
var gDescriptionElement;
var gInsertNewAuthor = false;
var gInsertNewDescription = false;
var gTitleWasEdited = false;
var gAuthorWasEdited = false;
var gDescWasEdited = false;



function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    window.close();
    return;
  }

  gDialog.PageLocation     = document.getElementById("PageLocation");
  gDialog.PageModDate      = document.getElementById("PageModDate");
  gDialog.TitleInput       = document.getElementById("TitleInput");
  gDialog.AuthorInput      = document.getElementById("AuthorInput");
  gDialog.DescriptionInput = document.getElementById("DescriptionInput");
  
  
  
  var location = GetDocumentUrl();
  var lastmodString = GetString("Unknown");

  if (!IsUrlAboutBlank(location))
  {
    
    gDialog.PageLocation.setAttribute("value", StripPassword(location));

    
    
    var lastmod;
    try {
      lastmod = editor.document.lastModified;  
    } catch (e) {}
    
    if(Date.parse(lastmod))
    {
      try {
        const nsScriptableDateFormat_CONTRACTID = "@mozilla.org/intl/scriptabledateformat;1";
        const nsIScriptableDateFormat = Components.interfaces.nsIScriptableDateFormat;
        var dateService = Components.classes[nsScriptableDateFormat_CONTRACTID]
         .getService(nsIScriptableDateFormat);

        var lastModDate = new Date();
        lastModDate.setTime(Date.parse(lastmod));
        lastmodString =  dateService.FormatDateTime("", 
                                      dateService.dateFormatLong,
                                      dateService.timeFormatSeconds,
                                      lastModDate.getFullYear(),
                                      lastModDate.getMonth()+1,
                                      lastModDate.getDate(),
                                      lastModDate.getHours(),
                                      lastModDate.getMinutes(),
                                      lastModDate.getSeconds());
      } catch (e) {}
    }
  }
  gDialog.PageModDate.value = lastmodString;

  gAuthorElement = GetMetaElement("author");
  if (!gAuthorElement)
  {
    gAuthorElement = CreateMetaElement("author");
    if (!gAuthorElement)
    {
      window.close();
      return;
    }
    gInsertNewAuthor = true;
  }

  gDescriptionElement = GetMetaElement("description");
  if (!gDescriptionElement)
  {
    gDescriptionElement = CreateMetaElement("description");
    if (!gDescriptionElement)
      window.close();

    gInsertNewDescription = true;
  }
  
  InitDialog();

  SetTextboxFocus(gDialog.TitleInput);

  SetWindowLocation();
}

function InitDialog()
{
  gDialog.TitleInput.value = GetDocumentTitle();

  var gAuthor = TrimString(gAuthorElement.getAttribute("content"));
  if (!gAuthor)
  {
    
    var prefs = GetPrefs();
    if (prefs) 
      gAuthor = prefs.getCharPref("editor.author");
  }
  gDialog.AuthorInput.value = gAuthor;
  gDialog.DescriptionInput.value = gDescriptionElement.getAttribute("content");
}

function TextboxChanged(ID)
{
  switch(ID)
  {
    case "TitleInput":
      gTitleWasEdited = true;
      break;
    case "AuthorInput":
      gAuthorWasEdited = true;
      break;
    case "DescriptionInput":
      gDescWasEdited = true;
      break;
  }
}

function ValidateData()
{
  gNewTitle = TrimString(gDialog.TitleInput.value);
  gAuthor = TrimString(gDialog.AuthorInput.value);
  gDescription = TrimString(gDialog.DescriptionInput.value);
  return true;
}

function onAccept()
{
  if (ValidateData())
  {
    var editor = GetCurrentEditor();
    editor.beginTransaction();

    
    
    if (gTitleWasEdited)
      SetDocumentTitle(gNewTitle);
    
    if (gAuthorWasEdited)
      SetMetaElementContent(gAuthorElement, gAuthor, gInsertNewAuthor, false);

    if (gDescWasEdited)
      SetMetaElementContent(gDescriptionElement, gDescription, gInsertNewDescription, false);

    editor.endTransaction();

    SaveWindowLocation();
    return true; 
  }
  return false;
}


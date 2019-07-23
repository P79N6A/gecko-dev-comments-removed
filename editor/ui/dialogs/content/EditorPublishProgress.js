





































var gInProgress = true;
var gPublishData;
var gPersistObj;
var gTotalFileCount = 0;
var gSucceededCount = 0;
var gFinished = false;
var gPublishingFailed = false;
var gFileNotFound = false;
var gStatusMessage="";

var gTimerID;
var gAllowEnterKey = false;




const kNetReset = 2152398868; 
const kFileNotFound = 2152857618;
const kNotConnected = 2152398860; 
const kConnectionRefused = 2152398861; 
const kNetTimeout = 2152398862; 
const kNoConnectionOrTimeout = 2152398878;
const kPortAccessNotAllowed = 2152398867; 
const kOffline = 2152398865; 
const kDiskFull = 2152857610;
const kNoDeviceSpace = 2152857616;
const kNameTooLong = 2152857617;
const kAccessDenied = 2152857621;



























function Startup()
{
  gPublishData = window.arguments[0];
  if (!gPublishData)
  {
    dump("No publish data!\n");
    window.close();
    return;
  }

  gDialog.FileList           = document.getElementById("FileList");
  gDialog.FinalStatusMessage = document.getElementById("FinalStatusMessage");
  gDialog.StatusMessage      = document.getElementById("StatusMessage");
  gDialog.KeepOpen           = document.getElementById("KeepOpen");
  gDialog.Close              = document.documentElement.getButton("cancel");

  SetWindowLocation();
  var title = GetDocumentTitle();
  if (!title)
    title = "("+GetString("untitled")+")";
  document.title = GetString("PublishProgressCaption").replace(/%title%/, title);

  document.getElementById("PublishToSite").value = 
    GetString("PublishToSite").replace(/%title%/, TruncateStringAtWordEnd(gPublishData.siteName, 25)); 

  
  document.getElementById("PublishUrl").value = gPublishData.publishUrl;
  
  
  if (gPublishData.docDir || gPublishData.otherDir)
  {
    if (gPublishData.docDir)
      document.getElementById("docDir").value = gPublishData.docDir;
    else
      document.getElementById("DocSubdir").hidden = true;
      
    if (gPublishData.publishOtherFiles && gPublishData.otherDir)
      document.getElementById("otherDir").value = gPublishData.otherDir;
    else
      document.getElementById("OtherSubdir").hidden = true;
  }
  else
    document.getElementById("Subdirectories").hidden = true;

  
  SetProgressStatus(gPublishData.filename, "busy");

  if (gPublishData.publishOtherFiles)
  {
    
    gDialog.FileList.setAttribute("rows", 5);
    window.sizeToContent();
  }

  
  gPersistObj = window.opener.StartPublishing();
}




function SetProgressStatusCancel()
{
  var listitems = document.getElementsByTagName("listitem");
  if (!listitems)
    return;

  for (var i=0; i < listitems.length; i++)
  {
    var attr = listitems[i].getAttribute("progress");
    if (attr != "done" && attr != "failed")
      listitems[i].setAttribute("progress", "failed");
  }
}




function SetProgressStatus(filename, status)
{
  if (!filename)
    return false;

  if (!status)
    status = "busy";

  
  
  var listitems = document.getElementsByTagName("listitem");
  if (listitems)
  {
    for (var i=0; i < listitems.length; i++)
    {
      if (listitems[i].getAttribute("label") == filename)
      {
        listitems[i].setAttribute("progress", status);
        return true;
      }
    }
  }
  
  gTotalFileCount++;

  var listitem = document.createElementNS(XUL_NS, "listitem");
  if (listitem)
  {
    listitem.setAttribute("class", "listitem-iconic progressitem");
    
    listitem.setAttribute("progress", status);
    listitem.setAttribute("label", filename);
    gDialog.FileList.appendChild(listitem);
  }
  return false;
}

function SetProgressFinished(filename, networkStatus)
{
  var abortPublishing = false;
  if (filename)
  {
    var status = networkStatus ? "failed" : "done";
    if (networkStatus == 0)
      gSucceededCount++;

    SetProgressStatus(filename, status);
  }

  if (networkStatus != 0) 
  {
    
    abortPublishing = networkStatus != kFileNotFound;

    
    if (abortPublishing)
    {
      gPublishingFailed = true;
      SetProgressStatusCancel();
      gDialog.FinalStatusMessage.value = GetString("PublishFailed");
    }

    switch (networkStatus)
    {
      case kFileNotFound:
        gFileNotFound = true;
        if (filename)
          gStatusMessage = GetString("FileNotFound").replace(/%file%/, filename);
        break;
      case kNetReset:
        
        
        var dir = (gPublishData.filename == filename) ? 
                     gPublishData.docDir : gPublishData.otherDir;

        if (dir)
        {
          
          
          gStatusMessage = GetString("SubdirDoesNotExist").replace(/%dir%/, dir.slice(0, dir.length-1));
          gStatusMessage = gStatusMessage.replace(/%file%/, filename);

          
          
          
          
          
          RemovePublishSubdirectoryFromPrefs(gPublishData, dir);
        }
        else if (filename)
          gStatusMessage = GetString("FilenameIsSubdir").replace(/%file%/, filename);

        break;
      case kNotConnected:
      case kConnectionRefused:
      case kNetTimeout:
      case kNoConnectionOrTimeout:
      case kPortAccessNotAllowed:
        gStatusMessage = GetString("ServerNotAvailable");
        break;
      case kOffline:
        gStatusMessage = GetString("Offline");
        break;
      case kDiskFull:
      case kNoDeviceSpace:
        if (filename)
          gStatusMessage = GetString("DiskFull").replace(/%file%/, filename);
        break;
      case kNameTooLong:
        if (filename)
          gStatusMessage = GetString("NameTooLong").replace(/%file%/, filename);
        break;
      case kAccessDenied:
        if (filename)
          gStatusMessage = GetString("AccessDenied").replace(/%file%/, filename);
        break;
      case kUnknownType:
      default:
        gStatusMessage = GetString("UnknownPublishError")
        break;
    }
  }
  else if (!filename)
  {
    gFinished = true;

    document.documentElement.setAttribute("buttonlabelcancel",
      document.documentElement.getAttribute("buttonlabelclose"));

    if (!gStatusMessage)
      gStatusMessage = GetString(gPublishingFailed ? "UnknownPublishError" : "AllFilesPublished");

    
    AllowDefaultButton();

    if (gPublishingFailed || gFileNotFound)
    {
      
      
      document.getElementById("failureBox").hidden = false;
    }
  }

  if (gStatusMessage)
    SetStatusMessage(gStatusMessage);
}

function CheckKeepOpen()
{
  if (gTimerID)
  {
    clearTimeout(gTimerID);
    gTimerID = null;
  }
}

function onClose()
{
  if (!gFinished)
  {
    const nsIPromptService = Components.interfaces.nsIPromptService;
    var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                  .getService(nsIPromptService);
    const buttonFlags = (nsIPromptService.BUTTON_TITLE_IS_STRING *
                         nsIPromptService.BUTTON_POS_0) +
                        (nsIPromptService.BUTTON_TITLE_CANCEL *
                         nsIPromptService.BUTTON_POS_1);
    var button = promptService.confirmEx(window,
                                         GetString("CancelPublishTitle"),
                                         GetString("CancelPublishMessage"),
                                         buttonFlags,
                                         GetString("CancelPublishContinue"),
                                         null, null, null, {});
    if (button == 0)
      return false;
  }

  if (gTimerID)
  {
    clearTimeout(gTimerID);
    gTimerID = null;
  }

  if (!gFinished && gPersistObj)
  {
    try {
      gPersistObj.cancelSave();
    } catch (e) {}
  }
  SaveWindowLocation();

  
  window.opener.FinishPublishing();
  return true;
}

function AllowDefaultButton()
{
  gDialog.Close.setAttribute("default","true");
  gAllowEnterKey = true;
}

function onEnterKey()
{
  if (gAllowEnterKey)
    return CloseDialog();

  return false;
}

function RequestCloseDialog()
{
  
  SetProgressFinished(null, 0);

  if (!gDialog.KeepOpen.checked)
  {
    
    gTimerID = setTimeout("CloseDialog();", 3000);
  }

  
  
  
  if (!gPublishingFailed)
  {
    gDialog.FinalStatusMessage.value = GetString("PublishCompleted");
    if (gFileNotFound && gTotalFileCount-gSucceededCount)
    {
      
      gStatusMessage = 
        (GetString("FailedFileMsg").replace(/%x%/,(gTotalFileCount-gSucceededCount)))
          .replace(/%total%/,gTotalFileCount);

      SetStatusMessage(gStatusMessage);
    }
  }
}

function SetStatusMessage(message)
{
  
  
  if (gDialog.StatusMessage.firstChild)
  {
    gDialog.StatusMessage.firstChild.data = message;
  }
  else
  {
    var textNode = document.createTextNode(message);
    if (textNode)
      gDialog.StatusMessage.appendChild(textNode);
  }
  window.sizeToContent();
}

function CloseDialog()
{
  SaveWindowLocation();
  window.opener.FinishPublishing();
  try {
    window.close();
  } catch (e) {}
}

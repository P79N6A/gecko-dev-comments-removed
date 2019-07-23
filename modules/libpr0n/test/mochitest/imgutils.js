





function clearImageCache()
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var imageCache = Components.classes["@mozilla.org/image/cache;1"]
                             .getService(Components.interfaces.imgICache);
  imageCache.clearCache(false); 
}


function isFrameDecoded(id)
{
  return (getImageStatus(id) &
          Components.interfaces.imgIRequest.STATUS_FRAME_COMPLETE)
         ? true : false;
}


function isImageLoaded(id)
{
  return (getImageStatus(id) &
          Components.interfaces.imgIRequest.STATUS_LOAD_COMPLETE)
         ? true : false;
}


function getImageStatus(id)
{
  
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  
  var img = document.getElementById(id);

  
  img.QueryInterface(Components.interfaces.nsIImageLoadingContent);

  
  var request = img.getRequest(Components.interfaces
                                         .nsIImageLoadingContent
                                         .CURRENT_REQUEST);

  
  return request.imageStatus;
}



function forceDecode(id)
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  
  var img = document.getElementById(id);

  
  var canvas = document.createElement("canvas");

  
  var ctx = canvas.getContext("2d");
  ctx.drawImage(img, 0, 0);
}








const DISCARD_BRANCH_NAME = "image.cache.";
const DISCARD_PREF_NAME = "discard_timer_ms";

function setDiscardTimerPref(timeMS)
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);
  var branch = prefService.getBranch(DISCARD_BRANCH_NAME);
  if (timeMS != null)
    branch.setIntPref(DISCARD_PREF_NAME, timeMS);
  else if (branch.prefHasUserValue(DISCARD_PREF_NAME))
    branch.clearUserPref(DISCARD_PREF_NAME);
}

function getDiscardTimerPref()
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);
  var branch = prefService.getBranch(DISCARD_BRANCH_NAME);
  if (branch.prefHasUserValue(DISCARD_PREF_NAME))
    return branch.getIntPref("discard_timeout_ms");
  else
    return null;
}

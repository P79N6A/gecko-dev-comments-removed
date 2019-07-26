






function clearImageCache()
{
  var tools = SpecialPowers.Cc["@mozilla.org/image/tools;1"]
                             .getService(SpecialPowers.Ci.imgITools);
  var imageCache = tools.getImgCacheForDocument(window.document);
  imageCache.clearCache(false); 
}


function isFrameDecoded(id)
{
  return (getImageStatus(id) &
          SpecialPowers.Ci.imgIRequest.STATUS_FRAME_COMPLETE)
         ? true : false;
}


function isImageLoaded(id)
{
  return (getImageStatus(id) &
          SpecialPowers.Ci.imgIRequest.STATUS_LOAD_COMPLETE)
         ? true : false;
}


function getImageStatus(id)
{
  
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  
  var img = SpecialPowers.wrap(document.getElementById(id));

  
  img.QueryInterface(SpecialPowers.Ci.nsIImageLoadingContent);

  
  var request = img.getRequest(SpecialPowers.Ci
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









const DISCARD_ENABLED_PREF = {name: "discardable", branch: "image.mem.", type: "bool"};
const DECODEONDRAW_ENABLED_PREF = {name: "decodeondraw", branch: "image.mem.", type: "bool"};
const DISCARD_TIMEOUT_PREF = {name: "min_discard_timeout_ms", branch: "image.mem.", type: "int"};

function setImagePref(pref, val)
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);
  var branch = prefService.getBranch(pref.branch);
  if (val != null) {
    switch(pref.type) {
      case "bool":
        branch.setBoolPref(pref.name, val);
        break;
      case "int":
        branch.setIntPref(pref.name, val);
        break;
      default:
        throw new Error("Unknown pref type");
    }
  }
  else if (branch.prefHasUserValue(pref.name))
    branch.clearUserPref(pref.name);
}

function getImagePref(pref)
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);
  var branch = prefService.getBranch(pref.branch);
  if (branch.prefHasUserValue(pref.name)) {
    switch (pref.type) {
      case "bool":
        return branch.getBoolPref(pref.name);
      case "int":
        return branch.getIntPref(pref.name);
      default:
        throw new Error("Unknown pref type");
    }
  }
  else
    return null;
}


function ImageDecoderObserverStub()
{
  this.sizeAvailable = function sizeAvailable(aRequest)   {}
  this.frameComplete = function frameComplete(aRequest)   {}
  this.decodeComplete = function decodeComplete(aRequest) {}
  this.loadComplete = function loadComplete(aRequest)     {}
  this.frameUpdate = function frameUpdate(aRequest)       {}
  this.discard = function discard(aRequest)               {}
  this.isAnimated = function isAnimated(aRequest)         {}
}

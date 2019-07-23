



































EXPORTED_SYMBOLS = [ "DownloadUtils" ];




























const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils
Cu.import("resource://gre/modules/PluralForm.jsm");

const kDownloadProperties =
  "chrome://mozapps/locale/downloads/downloads.properties";



let gStr = {
  statusFormat: "statusFormat2",
  transferSameUnits: "transferSameUnits",
  transferDiffUnits: "transferDiffUnits",
  transferNoTotal: "transferNoTotal",
  timePair: "timePair",
  timeLeftSingle: "timeLeftSingle",
  timeLeftDouble: "timeLeftDouble",
  timeFewSeconds: "timeFewSeconds",
  timeUnknown: "timeUnknown",
  doneScheme: "doneScheme",
  doneFileScheme: "doneFileScheme",
  units: ["bytes", "kilobyte", "megabyte", "gigabyte"],
  
  timeUnits: ["seconds", "minutes", "hours", "days"],
};


let (getStr = Cc["@mozilla.org/intl/stringbundle;1"].
              getService(Ci.nsIStringBundleService).
              createBundle(kDownloadProperties).
              GetStringFromName) {
  for (let [name, value] in Iterator(gStr)) {
    try {
      gStr[name] = typeof value == "string" ? getStr(value) : value.map(getStr);
    } catch (e) {
      log(["Couldn't get string '", name, "' from property '", value, "'"]);
    }
  }
}

let DownloadUtils = {
  













  getDownloadStatus: function(aCurrBytes, aMaxBytes, aSpeed, aLastSec)
  {
    if (isNil(aMaxBytes))
      aMaxBytes = -1;
    if (isNil(aSpeed))
      aSpeed = -1;
    if (isNil(aLastSec))
      aLastSec = Infinity;

    
    let seconds = (aSpeed > 0) && (aMaxBytes > 0) ?
      (aMaxBytes - aCurrBytes) / aSpeed : -1;

    
    let (transfer = DownloadUtils.getTransferTotal(aCurrBytes, aMaxBytes)) {
      
      status = replaceInsert(gStr.statusFormat, 1, transfer);
    }

    
    let ([rate, unit] = DownloadUtils.convertByteUnits(aSpeed)) {
      
      status = replaceInsert(status, 2, rate);
      
      status = replaceInsert(status, 3, unit);
    }

    
    let ([timeLeft, newLast] = DownloadUtils.getTimeLeft(seconds, aLastSec)) {
      
      status = replaceInsert(status, 4, timeLeft);

      return [status, newLast];
    }
  },

  










  getTransferTotal: function(aCurrBytes, aMaxBytes)
  {
    if (isNil(aMaxBytes))
      aMaxBytes = -1;

    let [progress, progressUnits] = DownloadUtils.convertByteUnits(aCurrBytes);
    let [total, totalUnits] = DownloadUtils.convertByteUnits(aMaxBytes);

    
    let transfer;
    if (total < 0)
      transfer = gStr.transferNoTotal;
    else if (progressUnits == totalUnits)
      transfer = gStr.transferSameUnits;
    else
      transfer = gStr.transferDiffUnits;

    transfer = replaceInsert(transfer, 1, progress);
    transfer = replaceInsert(transfer, 2, progressUnits);
    transfer = replaceInsert(transfer, 3, total);
    transfer = replaceInsert(transfer, 4, totalUnits);

    return transfer;
  },

  











  getTimeLeft: function(aSeconds, aLastSec)
  {
    if (isNil(aLastSec))
      aLastSec = Infinity;

    if (aSeconds < 0)
      return [gStr.timeUnknown, aLastSec];

    
    
    
    if (aSeconds > aLastSec / 2) {
      
      
      let (diff = aSeconds - aLastSec) {
        aSeconds = aLastSec + (diff < 0 ? .3 : .1) * diff;
      }

      
      
      let diff = aSeconds - aLastSec;
      let diffPct = diff / aLastSec * 100;
      if (Math.abs(diff) < 5 || Math.abs(diffPct) < 5)
        aSeconds = aLastSec - (diff < 0 ? .4 : .2);
    }

    
    let timeLeft;
    if (aSeconds < 4) {
      
      timeLeft = gStr.timeFewSeconds;
    } else {
      
      let [time1, unit1, time2, unit2] =
        DownloadUtils.convertTimeUnits(aSeconds);

      let pair1 = replaceInsert(gStr.timePair, 1, time1);
      pair1 = replaceInsert(pair1, 2, unit1);
      let pair2 = replaceInsert(gStr.timePair, 1, time2);
      pair2 = replaceInsert(pair2, 2, unit2);

      
      if (aSeconds < 3600 || time2 == 0) {
        timeLeft = replaceInsert(gStr.timeLeftSingle, 1, pair1);
      } else {
        
        timeLeft = replaceInsert(gStr.timeLeftDouble, 1, pair1);
        timeLeft = replaceInsert(timeLeft, 2, pair2);
      }
    }

    return [timeLeft, aSeconds];
  },

  







  getURIHost: function(aURIString)
  {
    let ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    let eTLDService = Cc["@mozilla.org/network/effective-tld-service;1"].
                      getService(Ci.nsIEffectiveTLDService);
    let idnService = Cc["@mozilla.org/network/idn-service;1"].
                     getService(Ci.nsIIDNService);

    
    let uri = ioService.newURI(aURIString, null, null);

    
    if (uri instanceof Ci.nsINestedURI)
      uri = uri.innermostURI;

    let fullHost;
    try {
      
      fullHost = uri.host;
    } catch (e) {
      fullHost = "";
    }

    let displayHost;
    try {
      
      let baseDomain = eTLDService.getBaseDomain(uri);

      
      displayHost = idnService.convertToDisplayIDN(baseDomain, {});
    } catch (e) {
      
      displayHost = fullHost;
    }

    
    if (uri.scheme == "file") {
      
      displayHost = gStr.doneFileScheme;
      fullHost = displayHost;
    } else if (displayHost.length == 0) {
      
      displayHost = replaceInsert(gStr.doneScheme, 1, uri.scheme);
      fullHost = displayHost;
    } else if (uri.port != -1) {
      
      let port = ":" + uri.port;
      displayHost += port;
      fullHost += port;
    }

    return [displayHost, fullHost];
  },

  







  convertByteUnits: function(aBytes)
  {
    let unitIndex = 0;

    
    
    while ((aBytes >= 999.5) && (unitIndex < gStr.units.length - 1)) {
      aBytes /= 1024;
      unitIndex++;
    }

    
    
    aBytes = aBytes.toFixed((aBytes > 0) && (aBytes < 100) ? 1 : 0);

    return [aBytes, gStr.units[unitIndex]];
  },

  







  convertTimeUnits: function(aSecs)
  {
    
    
    let timeSize = [60, 60, 24];

    let time = aSecs;
    let scale = 1;
    let unitIndex = 0;

    
    
    while ((unitIndex < timeSize.length) && (time >= timeSize[unitIndex])) {
      time /= timeSize[unitIndex];
      scale *= timeSize[unitIndex];
      unitIndex++;
    }

    let value = convertTimeUnitsValue(time);
    let units = convertTimeUnitsUnits(value, unitIndex);

    let extra = aSecs - value * scale;
    let nextIndex = unitIndex - 1;

    
    for (let index = 0; index < nextIndex; index++)
      extra /= timeSize[index];

    let value2 = convertTimeUnitsValue(extra);
    let units2 = convertTimeUnitsUnits(value2, nextIndex);

    return [value, units, value2, units2];
  },
};








function convertTimeUnitsValue(aTime)
{
  return Math.floor(aTime);
}










function convertTimeUnitsUnits(aTime, aIndex)
{
  return PluralForm.get(aTime, gStr.timeUnits[aIndex]);
}












function replaceInsert(aText, aIndex, aValue)
{
  return aText.replace("#" + aIndex, aValue);
}








function isNil(aArg)
{
  return (aArg == null) || (aArg == undefined);
}







function log(aMsg)
{
  let msg = "DownloadUtils.jsm: " + (aMsg.join ? aMsg.join("") : aMsg);
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).
    logStringMessage(msg);
  dump(msg + "\n");
}

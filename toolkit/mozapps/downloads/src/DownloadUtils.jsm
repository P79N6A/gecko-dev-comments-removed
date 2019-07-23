



































EXPORTED_SYMBOLS = [ "DownloadUtils" ];

























const Cc = Components.classes;
const Ci = Components.interfaces;

const kDownloadProperties =
  "chrome://mozapps/locale/downloads/downloads.properties";



let gStr = {
  statusFormat: "statusFormat2",
  transferSameUnits: "transferSameUnits",
  transferDiffUnits: "transferDiffUnits",
  transferNoTotal: "transferNoTotal",
  timeMinutesLeft: "timeMinutesLeft",
  timeSecondsLeft: "timeSecondsLeft",
  timeFewSeconds: "timeFewSeconds",
  timeUnknown: "timeUnknown",
  doneScheme: "doneScheme",
  doneFileScheme: "doneFileScheme",
  units: ["bytes", "kilobyte", "megabyte", "gigabyte"],
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
      Math.ceil((aMaxBytes - aCurrBytes) / aSpeed) : -1;

    
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

    
    
    
    let (diff = aSeconds - aLastSec) {
      if (diff > 0 && diff <= 10)
        aSeconds = aLastSec;
    }

    
    let timeLeft;
    if (aSeconds < 4) {
      
      timeLeft = gStr.timeFewSeconds;
    } else if (aSeconds <= 60) {
      
      timeLeft = replaceInsert(gStr.timeSecondsLeft, 1, aSeconds);
    } else {
      
      timeLeft = replaceInsert(gStr.timeMinutesLeft, 1,
                               Math.ceil(aSeconds / 60));
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
};












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

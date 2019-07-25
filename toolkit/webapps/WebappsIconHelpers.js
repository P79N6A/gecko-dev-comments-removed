
















function getBiggestIconURL(aIcons) {
  if (!aIcons) {
    return "chrome://browser/skin/webapps-64.png";
  }

  let iconSizes = Object.keys(aIcons);
  if (iconSizes.length == 0) {
    return "chrome://browser/skin/webapps-64.png";
  }
  iconSizes.sort(function(a, b) a - b);
  return aIcons[iconSizes.pop()];
}













function getIconForApp(aShell, callback) {
  let iconURI = aShell.iconURI;
  let mimeService = Cc["@mozilla.org/mime;1"]
                      .getService(Ci.nsIMIMEService);

  let mimeType;
  try {
    let tIndex = iconURI.path.indexOf(";");
    if("data" == iconURI.scheme && tIndex != -1) {
      mimeType = iconURI.path.substring(0, tIndex);
    } else {
      mimeType = mimeService.getTypeFromURI(iconURI);
    }
  } catch(e) {
    throw("getIconFromURI - Failed to determine MIME type");
  }

  try {
    let listener;
    if(aShell.useTmpForIcon) {
      let downloadObserver = {
        onDownloadComplete: function(downloader, request, cx, aStatus, file) {
          
          onIconDownloaded(aShell, mimeType, aStatus, file, callback, downloader);
        }
      };

      let tmpIcon = Services.dirsvc.get("TmpD", Ci.nsIFile);
      tmpIcon.append("tmpicon." + mimeService.getPrimaryExtension(mimeType, ""));
      tmpIcon.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

      listener = Cc["@mozilla.org/network/downloader;1"]
                   .createInstance(Ci.nsIDownloader);
      listener.init(downloadObserver, tmpIcon);
    } else {
      let pipe = Cc["@mozilla.org/pipe;1"]
                   .createInstance(Ci.nsIPipe);
      pipe.init(true, true, 0, 0xffffffff, null);

      listener = Cc["@mozilla.org/network/simple-stream-listener;1"]
                   .createInstance(Ci.nsISimpleStreamListener);
      listener.init(pipe.outputStream, {
          onStartRequest: function() {},
          onStopRequest: function(aRequest, aContext, aStatusCode) {
            pipe.outputStream.close();
            onIconDownloaded(aShell, mimeType, aStatusCode, pipe.inputStream, callback);
         }
      });
    }

    let channel = NetUtil.newChannel(iconURI);
    let CertUtils = { };
    Cu.import("resource://gre/modules/CertUtils.jsm", CertUtils);
    
    channel.notificationCallbacks = new CertUtils.BadCertHandler(true);

    channel.asyncOpen(listener, null);
  } catch(e) {
    throw("getIconFromURI - Failure getting icon (" + e + ")");
  }
}

function onIconDownloaded(aShell, aMimeType, aStatusCode, aIcon, aCallback) {
  if (Components.isSuccessCode(aStatusCode)) {
    aShell.processIcon(aMimeType, aIcon, aCallback);
  } else {
    aCallback.call(aShell);
  }
}

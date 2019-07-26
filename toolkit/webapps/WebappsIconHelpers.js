



const DEFAULT_ICON_URL = "chrome://global/skin/icons/webapps-64.png";














function getBiggestIconURL(aIcons) {
  if (!aIcons) {
    return DEFAULT_ICON_URL;
  }

  let iconSizes = Object.keys(aIcons);
  if (iconSizes.length == 0) {
    return DEFAULT_ICON_URL;
  }
  iconSizes.sort(function(a, b) a - b);
  return aIcons[iconSizes.pop()];
}


function downloadIcon(aIconURI) {
  let deferred = Promise.defer();

  let mimeService = Cc["@mozilla.org/mime;1"].getService(Ci.nsIMIMEService);
  let mimeType;
  try {
    let tIndex = aIconURI.path.indexOf(";");
    if("data" == aIconURI.scheme && tIndex != -1) {
      mimeType = aIconURI.path.substring(0, tIndex);
    } else {
      mimeType = mimeService.getTypeFromURI(aIconURI);
     }
  } catch(e) {
    deferred.reject("Failed to determine icon MIME type: " + e);
    return deferred.promise;
  }

  function onIconDownloaded(aStatusCode, aIcon) {
    if (Components.isSuccessCode(aStatusCode)) {
      deferred.resolve([ mimeType, aIcon ]);
    } else {
      deferred.reject("Failure downloading icon: " + aStatusCode);
    }
  }

  try {
#ifdef XP_MACOSX
    let downloadObserver = {
      onDownloadComplete: function(downloader, request, cx, aStatus, file) {
        onIconDownloaded(aStatus, file);
      }
    };

    let tmpIcon = Services.dirsvc.get("TmpD", Ci.nsIFile);
    tmpIcon.append("tmpicon." + mimeService.getPrimaryExtension(mimeType, ""));
    tmpIcon.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("666", 8));

    let listener = Cc["@mozilla.org/network/downloader;1"]
                     .createInstance(Ci.nsIDownloader);
    listener.init(downloadObserver, tmpIcon);
#else
    let pipe = Cc["@mozilla.org/pipe;1"]
                 .createInstance(Ci.nsIPipe);
    pipe.init(true, true, 0, 0xffffffff, null);

    let listener = Cc["@mozilla.org/network/simple-stream-listener;1"]
                     .createInstance(Ci.nsISimpleStreamListener);
    listener.init(pipe.outputStream, {
        onStartRequest: function() {},
        onStopRequest: function(aRequest, aContext, aStatusCode) {
          pipe.outputStream.close();
          onIconDownloaded(aStatusCode, pipe.inputStream);
       }
    });
#endif

    let channel = NetUtil.newChannel(aIconURI);
    let { BadCertHandler } = Cu.import("resource://gre/modules/CertUtils.jsm", {});
    
    channel.notificationCallbacks = new BadCertHandler(true);

    channel.asyncOpen(listener, null);
  } catch(e) {
    deferred.reject("Failure initiating download of icon: " + e);
  }

  return deferred.promise;
}

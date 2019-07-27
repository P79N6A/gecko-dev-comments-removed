






























const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const CC = Components.Constructor;

Cu.import('resource://gre/modules/Webapps.jsm'); 
Cu.import('resource://gre/modules/AppsUtils.jsm'); 
Cu.import('resource://gre/modules/Services.jsm'); 



function fetchManifest(manifestURL) {
  return new Promise(function(resolve, reject) {
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", manifestURL, true);
    xhr.responseType = "json";

    xhr.addEventListener("load", function() {
      if (xhr.status == 200) {
        resolve(xhr.response);
      } else {
        reject();
      }
    });

    xhr.addEventListener("error", function() {
      reject();
    });

    xhr.send(null);
  });
}






function installApp(req) {
  fetchManifest(req.manifestURL).then(function(manifestObj) {
    var data = {
      
      app: AppsUtils.cloneAppObject({
        installOrigin: req.origin,
        origin: req.origin,
        manifestURL: req.manifestURL,
        appStatus: AppsUtils.getAppManifestStatus(manifestObj),
        receipts: [],
        categories: []
      }),

      from: req.origin, 
      oid: 0, 
      requestID: 0, 
      appId: 0, 
      isBrowser: false,
      isPackage: false, 
      
      forceSuccessAck: false
      
    };
    
    data.app.manifest = manifestObj;

    return DOMApplicationRegistry.confirmInstall(data).then(
      function() {
        var appId =
          DOMApplicationRegistry.getAppLocalIdByManifestURL(req.manifestURL);
        
        
        DOMApplicationRegistry.updatePermissionsForApp(
          appId,
           true,
           true);

        sendAsyncMessage(
          'installed',
          {
            appId: appId,
            manifestURL: req.manifestURL,
            manifest: manifestObj
          });
      },
      function(err) {
        sendAsyncMessage('installed', false);
      });
  });
}

function uninstallApp(appInfo) {
  DOMApplicationRegistry.uninstall(appInfo.manifestURL).then(
    function() {
      sendAsyncMessage('uninstalled', true);
    },
    function() {
      sendAsyncMessage('uninstalled', false);
    });
}

var activeIframe = null;






function runApp(appInfo) {
  let shellDomWindow = Services.wm.getMostRecentWindow('navigator:browser');
  let sysAppFrame = shellDomWindow.document.body.querySelector('#systemapp');
  let sysAppDoc = sysAppFrame.contentDocument;

  let siblingFrame = sysAppDoc.body.querySelector('#test-container');

  let ifr = activeIframe = sysAppDoc.createElement('iframe');
  ifr.setAttribute('mozbrowser', 'true');
  ifr.setAttribute('remote', 'true');
  ifr.setAttribute('mozapp', appInfo.manifestURL);

  ifr.addEventListener('mozbrowsershowmodalprompt', function(evt) {
    var message = evt.detail.message;
    
    if (activeIframe) {
      sendAsyncMessage('appMessage', message);
    }
  }, false);
  ifr.addEventListener('mozbrowsererror', function(evt) {
    if (activeIframe) {
      sendAsyncMessage('appError', { message: '' + evt.detail });
    }
  });

  ifr.setAttribute('src', appInfo.manifest.launch_path);
  siblingFrame.parentElement.appendChild(ifr);
}

function closeApp() {
  if (activeIframe) {
    activeIframe.parentElement.removeChild(activeIframe);
    activeIframe = null;
  }
}

addMessageListener('install', installApp);
addMessageListener('uninstall', uninstallApp);
addMessageListener('run', runApp);
addMessageListener('close', closeApp);

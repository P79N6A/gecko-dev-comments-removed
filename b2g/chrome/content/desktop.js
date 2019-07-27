



let browserWindow = Services.wm.getMostRecentWindow("navigator:browser");
let isMulet = "ResponsiveUI" in browserWindow;



function enableTouch() {
  let require = Cu.import('resource://gre/modules/devtools/Loader.jsm', {})
                  .devtools.require;
  let { TouchEventHandler } = require('devtools/touch-events');
  let chromeEventHandler = window.QueryInterface(Ci.nsIInterfaceRequestor)
                                 .getInterface(Ci.nsIWebNavigation)
                                 .QueryInterface(Ci.nsIDocShell)
                                 .chromeEventHandler || window;
  let touchEventHandler = new TouchEventHandler(chromeEventHandler);
  touchEventHandler.start();
}

function setupButtons() {
  let homeButton = document.getElementById('home-button');
  if (!homeButton) {
    
    
    return;
  }
  
  
  
  homeButton.addEventListener('touchstart', function() {
    let window = shell.contentBrowser.contentWindow;
    let e = new window.KeyboardEvent('keydown', {key: 'Home'});
    window.dispatchEvent(e);
    homeButton.classList.add('active');
  });
  homeButton.addEventListener('touchend', function() {
    let window = shell.contentBrowser.contentWindow;
    let e = new window.KeyboardEvent('keyup', {key: 'Home'});
    window.dispatchEvent(e);
    homeButton.classList.remove('active');
  });

  Cu.import("resource://gre/modules/GlobalSimulatorScreen.jsm");
  let rotateButton = document.getElementById('rotate-button');
  rotateButton.addEventListener('touchstart', function () {
    rotateButton.classList.add('active');
  });
  rotateButton.addEventListener('touchend', function() {
    GlobalSimulatorScreen.flipScreen();
    rotateButton.classList.remove('active');
  });
}

function setupStorage() {
  let directory = null;

  
  try {
    let service = Cc['@mozilla.org/commandlinehandler/general-startup;1?type=b2gcmds'].getService(Ci.nsISupports);
    let args = service.wrappedJSObject.cmdLine;
    if (args) {
      let path = args.handleFlagWithParam('storage-path', false);
      directory = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsIFile);
      directory.initWithPath(path);
    }
  } catch(e) {
    directory = null;
  }

  
  if (!directory) {
    directory = Services.dirsvc.get('ProfD', Ci.nsIFile);
    directory.append('storage');
    if (!directory.exists()) {
      directory.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt("755", 8));
    }
  }
  dump("Set storage path to: " + directory.path + "\n");

  
  Services.prefs.setCharPref('device.storage.overrideRootDir', directory.path);
}

function checkDebuggerPort() {
  
  
  
  

  
  let args;
  try {
    let service = Cc["@mozilla.org/commandlinehandler/general-startup;1?type=b2gcmds"].getService(Ci.nsISupports);
    args = service.wrappedJSObject.cmdLine;
  } catch(e) {}

  if (!args) {
    return;
  }

  let dbgport;
  try {
    dbgport = args.handleFlagWithParam('start-debugger-server', false);
  } catch(e) {}

  if (dbgport) {
    dump('Opening debugger server on ' + dbgport + '\n');
    Services.prefs.setCharPref('devtools.debugger.unix-domain-socket', dbgport);
    navigator.mozSettings.createLock().set(
      {'debugger.remote-mode': 'adb-devtools'});
  }
}


function initResponsiveDesign() {
  Cu.import('resource:///modules/devtools/responsivedesign.jsm');
  ResponsiveUIManager.on('on', function(event, {tab:tab}) {
    let responsive = ResponsiveUIManager.getResponsiveUIForTab(tab);
    let document = tab.ownerDocument;

    
    if (tab.linkedBrowser.contentWindow != window) {
      return;
    }

    
    responsive.transitionsEnabled = false;

    responsive.buildPhoneUI();

    responsive.rotatebutton.addEventListener('command', function (evt) {
      GlobalSimulatorScreen.flipScreen();
      evt.stopImmediatePropagation();
      evt.preventDefault();
    }, true);

    
    responsive.enableTouch();

    
    let width = 320, height = 480;
    
    
    width += 15*2; 
    width += 1*2; 
    height += 60; 
    height += 1; 
    responsive.setSize(width, height);
  });


  let mgr = browserWindow.ResponsiveUI.ResponsiveUIManager;
  mgr.toggle(browserWindow, browserWindow.gBrowser.selectedTab);

}

function openDevtools() {
  
  Services.prefs.setIntPref('devtools.toolbox.sidebar.width',
                            browserWindow.outerWidth - 550);
  Services.prefs.setCharPref('devtools.toolbox.host', 'side');
  let {gDevTools} = Cu.import('resource:///modules/devtools/gDevTools.jsm', {});
  let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
  let target = devtools.TargetFactory.forTab(browserWindow.gBrowser.selectedTab);
  gDevTools.showToolbox(target);
}

window.addEventListener('ContentStart', function() {
  
  if (!isMulet) {
    enableTouch();
  }
  setupButtons();
  checkDebuggerPort();
  setupStorage();
  
  
  if (isMulet) {
    initResponsiveDesign(browserWindow);
    openDevtools();
  }
});

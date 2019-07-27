



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
    shell.sendChromeEvent({type: 'home-button-press'});
    homeButton.classList.add('active');
  });
  homeButton.addEventListener('touchend', function() {
    shell.sendChromeEvent({type: 'home-button-release'});
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
    let responsive = tab.__responsiveUI;
    let document = tab.ownerDocument;

    
    if (tab.linkedBrowser.contentWindow != window) {
      return;
    }

    responsive.buildPhoneUI();

    responsive.rotatebutton.addEventListener('command', function (evt) {
      GlobalSimulatorScreen.flipScreen();
      evt.stopImmediatePropagation();
      evt.preventDefault();
    }, true);

    
    browserWindow.gBrowser.selectedTab.__responsiveUI.enableTouch();
  });

  
  let width = 320, height = 480;
  
  
  width += 15*2; 
  width += 1*2; 
  height += 60; 
  height += 1; 
  let args = {'width': width, 'height': height};
  let mgr = browserWindow.ResponsiveUI.ResponsiveUIManager;
  mgr.toggle(browserWindow, browserWindow.gBrowser.selectedTab);
  let responsive = browserWindow.gBrowser.selectedTab.__responsiveUI;
  responsive.setSize(width, height);

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
  
  
  if (isMulet) {
    initResponsiveDesign(browserWindow);
    openDevtools();
  }
});

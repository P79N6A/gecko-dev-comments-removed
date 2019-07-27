






let browserWindow = Services.wm.getMostRecentWindow("navigator:browser");
let isMulet = "ResponsiveUI" in browserWindow;
Cu.import("resource://gre/modules/GlobalSimulatorScreen.jsm");


window.addEventListener('ContentStart', function() {
  
  let shell = document.getElementById('shell');

  
  let browser = document.getElementById('systemapp');

  
  let windowUtils = window.QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Components.interfaces.nsIDOMWindowUtils);
  let hostDPI = windowUtils.displayDPI;

  let DEFAULT_SCREEN = '320x480';

  
  
  
  let screens = {
    iphone: {
      name: 'Apple iPhone', width:320, height:480,  dpi:163
    },
    ipad: {
      name: 'Apple iPad', width:1024, height:768,  dpi:132
    },
    nexus_s: {
      name: 'Samsung Nexus S', width:480, height:800, dpi:235
    },
    galaxy_s2: {
      name: 'Samsung Galaxy SII (I9100)', width:480, height:800, dpi:219
    },
    galaxy_nexus: {
      name: 'Samsung Galaxy Nexus', width:720, height:1280, dpi:316
    },
    galaxy_tab: {
      name: 'Samsung Galaxy Tab 10.1', width:800, height:1280, dpi:149
    },
    wildfire: {
      name: 'HTC Wildfire', width:240, height:320, dpi:125
    },
    tattoo: {
      name: 'HTC Tattoo', width:240, height:320, dpi:143
    },
    salsa: {
      name: 'HTC Salsa', width:320, height:480, dpi:170
    },
    chacha: {
      name: 'HTC ChaCha', width:320, height:480, dpi:222
    },
  };

  
  let args;
  try {
    let service = Cc["@mozilla.org/commandlinehandler/general-startup;1?type=b2gcmds"].getService(Ci.nsISupports);
    args = service.wrappedJSObject.cmdLine;
  } catch(e) {}

  let screenarg = null;

  
  try {
    if (args) {
      screenarg = args.handleFlagWithParam('screen', false);
    }

    
    if (screenarg === null && Services.prefs.prefHasUserValue('b2g.screen.size')) {
      screenarg = Services.prefs.getCharPref('b2g.screen.size');
    }

    
    if (screenarg === null)
      screenarg = DEFAULT_SCREEN;

    
    if (screenarg == '')
      usage();
  }
  catch(e) {
    
    usage();
  }
  
  
  if (screenarg === 'full') {
    shell.setAttribute('sizemode', 'fullscreen');
    return;
  } 

  let width, height, ratio = 1.0;

  if (screenarg in screens) {
    
    let screen = screens[screenarg];
    width = screen.width;
    height = screen.height;
    ratio = screen.ratio;
  } else {
    
    
    let match = screenarg.match(/^(\d+)x(\d+)(@(\d+(\.\d+)?))?$/);
    
    
    if (match == null)
      usage();
    
    
    width = parseInt(match[1], 10);
    height = parseInt(match[2], 10);
    if (match[4])
      ratio = parseFloat(match[4], 10);

    
    if (!width || !height || !ratio) {
      usage();
    }
  }

  Services.prefs.setCharPref('layout.css.devPixelsPerPx',
                             ratio == 1 ? -1 : ratio);
  let defaultOrientation = width < height ? 'portrait' : 'landscape';
  GlobalSimulatorScreen.mozOrientation = GlobalSimulatorScreen.screenOrientation = defaultOrientation;

  function resize() {
    GlobalSimulatorScreen.width = width;
    GlobalSimulatorScreen.height = height;

    
    
    let controls = document.getElementById('controls');
    let controlsHeight = controls ? controls.getBoundingClientRect().height : 0;

    if (isMulet) {
      let tab = browserWindow.gBrowser.selectedTab;
      let responsive = ResponsiveUIManager.getResponsiveUIForTab(tab);
      responsive.setSize(width + 16*2,
                         height + controlsHeight + 61);
    } else {
      let chromewidth = window.outerWidth - window.innerWidth;
      let chromeheight = window.outerHeight - window.innerHeight + controlsHeight;
      window.resizeTo(width + chromewidth,
                      height + chromeheight);
    }

    let frameWidth = width, frameHeight = height;

    
    
    
    let shouldFlip = GlobalSimulatorScreen.mozOrientation != GlobalSimulatorScreen.screenOrientation;

    if (shouldFlip) {
      frameWidth = height;
      frameHeight = width;
    }

    
    let style = browser.style;
    style.transform = '';
    style.height = 'calc(100% - ' + controlsHeight + 'px)';
    style.bottom = controlsHeight;

    style.width = frameWidth + "px";
    style.height = frameHeight + "px";

    if (shouldFlip) {
      
      let shift = Math.floor(Math.abs(frameWidth - frameHeight) / 2);
      style.transform +=
        ' rotate(0.25turn) translate(-' + shift + 'px, -' + shift + 'px)';
    }
  }

  
  resize();

  
  window.onresize = function() {
    let controls = document.getElementById('controls');
    let controlsHeight = controls ? controls.getBoundingClientRect().height : 0;

    width = window.innerWidth;
    height = window.innerHeight - controlsHeight;

    queueResize();
  };

  
  
  Services.obs.addObserver(function orientationChangeListener(subject) {
    let screen = subject.wrappedJSObject;
    let { mozOrientation, screenOrientation } = screen;

    
    
    if (screenOrientation != defaultOrientation) {
      let w = width;
      width = height;
      height = w;
    }
    defaultOrientation = screenOrientation;

    queueResize();
  }, 'simulator-adjust-window-size', false);

  
  
  let resizeTimeout;
  function queueResize() {
    if (resizeTimeout) {
      clearTimeout(resizeTimeout);
    }
    resizeTimeout = setTimeout(function () {
      resizeTimeout = null;
      resize();
    }, 0);
  }

  
  
  function print() {
    let dump_enabled =
      Services.prefs.getBoolPref('browser.dom.window.dump.enabled');

    if (!dump_enabled)
      Services.prefs.setBoolPref('browser.dom.window.dump.enabled', true);

    dump(Array.prototype.join.call(arguments, ' ') + '\n');

    if (!dump_enabled) 
      Services.prefs.setBoolPref('browser.dom.window.dump.enabled', false);
  }

  
  function usage() {
    
    let msg = 
      'The --screen argument specifies the desired resolution and\n' +
      'pixel density of the simulated device screen. Use it like this:\n' +
      '\t--screen=WIDTHxHEIGHT\t\t\t// E.g.: --screen=320x480\n' +
      '\t--screen=WIDTHxHEIGHT@DOTS_PER_INCH\t// E.g.: --screen=480x800@250\n' +
      '\t--screen=full\t\t\t\t// run in fullscreen mode\n' +
      '\nYou can also specify certain device names:\n';
    for(let p in screens)
      msg += '\t--screen=' + p + '\t// ' + screens[p].name + '\n';

    
    print(msg);

    
    Services.startup.quit(Ci.nsIAppStartup.eAttemptQuit);
  }
});

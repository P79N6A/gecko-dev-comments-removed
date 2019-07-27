






let browserWindow = Services.wm.getMostRecentWindow("navigator:browser");
let isMulet = "ResponsiveUI" in browserWindow;


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

  let rescale = false;

  
  if (screenarg[screenarg.length - 1] === '!') {
    rescale = true;
    screenarg = screenarg.substring(0, screenarg.length-1);
  }

  let width, height, dpi;

  if (screenarg in screens) {
    
    let screen = screens[screenarg];
    width = screen.width;
    height = screen.height;
    dpi = screen.dpi;
  } else {
    
    
    let match = screenarg.match(/^(\d+)x(\d+)(@(\d+))?$/);
    
    
    if (match == null)
      usage();
    
    
    width = parseInt(match[1], 10);
    height = parseInt(match[2], 10);
    if (match[4])
      dpi = parseInt(match[4], 10);
    else    
      dpi = hostDPI;

    
    if (!width || !height || !dpi)
      usage();
  }

  Cu.import("resource://gre/modules/GlobalSimulatorScreen.jsm");
  function resize(width, height, dpi, shouldFlip) {
    GlobalSimulatorScreen.width = width;
    GlobalSimulatorScreen.height = height;

    
    
    
    
    let scale = rescale ? hostDPI / dpi : 1;

    
    
    let controls = document.getElementById('controls');
    let controlsHeight = 0;
    if (controls) {
      controlsHeight = controls.getBoundingClientRect().height;
    }
    let chromewidth = window.outerWidth - window.innerWidth;
    let chromeheight = window.outerHeight - window.innerHeight + controlsHeight;
    if (isMulet) {
      let responsive = browserWindow.gBrowser.selectedTab.__responsiveUI;
      responsive.setSize((Math.round(width * scale) + 14*2),
                        (Math.round(height * scale) + controlsHeight + 61));
    } else {
      window.resizeTo(Math.round(width * scale) + chromewidth,
                      Math.round(height * scale) + chromeheight);
    }

    let frameWidth = width, frameHeight = height;
    if (shouldFlip) {
      frameWidth = height;
      frameHeight = width;
    }

    
    let style = browser.style;
    style.width = style.minWidth = style.maxWidth =
      frameWidth + 'px';
    style.height = style.minHeight = style.maxHeight =
      frameHeight + 'px';
    browser.setAttribute('flex', '0');  

    style.transformOrigin = '';
    style.transform = '';

    
    if (scale !== 1) {
      style.transformOrigin = 'top left';
      style.transform += ' scale(' + scale + ',' + scale + ')';
    }

    if (shouldFlip) {
      
      let shift = Math.floor(Math.abs(frameWidth-frameHeight) / 2);
      style.transform +=
        ' rotate(0.25turn) translate(-' + shift + 'px, -' + shift + 'px)';
    }

    
    
    
    Services.prefs.setIntPref('layout.css.dpi', dpi);
  }

  
  resize(width, height, dpi, false);

  let defaultOrientation = width < height ? 'portrait' : 'landscape';

  
  
  Services.obs.addObserver(function orientationChangeListener(subject) {
    let screen = subject.wrappedJSObject;
    let { mozOrientation, screenOrientation } = screen;

    let newWidth = width;
    let newHeight = height;
    
    
    if (screenOrientation != defaultOrientation) {
      newWidth = height;
      newHeight = width;
    }

    
    
    
    let shouldFlip = mozOrientation != screenOrientation;

    resize(newWidth, newHeight, dpi, shouldFlip);
  }, 'simulator-adjust-window-size', false);

  
  
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
    msg += 
      '\nAdd a ! to the end of a screen specification to rescale the\n' +
      'screen so that it is shown at actual size on your monitor:\n' +
      '\t--screen=nexus_s!\n' +
      '\t--screen=320x480@200!\n'
    ;

    
    print(msg);

    
    Services.startup.quit(Ci.nsIAppStartup.eAttemptQuit);
  }
});







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

function setupHomeButton() {
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
}

window.addEventListener('ContentStart', function() {
  enableTouch();
  setupHomeButton();
});

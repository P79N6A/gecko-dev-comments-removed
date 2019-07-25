





Components.utils.import("resource://gre/modules/Services.jsm");


const NS_ALERT_HORIZONTAL = 1;
const NS_ALERT_LEFT = 2;
const NS_ALERT_TOP = 4;

var gFinalSize;
var gCurrentSize = 1;

var gSlideIncrement = 1;
var gSlideTime = 10;
var gOpenTime = 3000; 
var gOrigin = 0; 
var gDisableSlideEffect = false;
 
var gAlertListener = null;
var gAlertTextClickable = false;
var gAlertCookie = "";

function prefillAlertInfo()
{
  
  
  
  
  
  
  
  

  switch (window.arguments.length)
  {
    default:
    case 7:
      gAlertListener = window.arguments[6];
    case 6:
      gOrigin = window.arguments[5];
    case 5:
      gAlertCookie = window.arguments[4];
    case 4:
      gAlertTextClickable = window.arguments[3];
      if (gAlertTextClickable) {
        document.getElementById('alertNotification').setAttribute('clickable', true);
        document.getElementById('alertTextLabel').setAttribute('clickable', true);
      }
    case 3:
      document.getElementById('alertTextLabel').setAttribute('value', window.arguments[2]);
    case 2:
      document.getElementById('alertTitleLabel').setAttribute('value', window.arguments[1]);
    case 1:
      document.getElementById('alertImage').setAttribute('src', window.arguments[0]);
    case 0:
      break;
  }
}

function onAlertLoad()
{
  gSlideIncrement     = Services.prefs.getIntPref("alerts.slideIncrement");
  gSlideTime          = Services.prefs.getIntPref("alerts.slideIncrementTime");
  gOpenTime           = Services.prefs.getIntPref("alerts.totalOpenTime");
  gDisableSlideEffect = Services.prefs.getBoolPref("alerts.disableSlidingEffect");

  var alertBox = document.getElementById("alertBox");
  
  
  
  
  if (gOrigin & NS_ALERT_HORIZONTAL)
  {
    if (gOrigin & NS_ALERT_LEFT) {
      document.documentElement.pack = "end";
      alertBox.setAttribute("origin", "left");
    } else {
      alertBox.setAttribute("origin", "right");
    }

    
    document.documentElement.orient = "horizontal";
  }
  else
  {
    if (gOrigin & NS_ALERT_TOP) {
      document.documentElement.pack = "end";
      alertBox.setAttribute("origin", "top");
    } else {
      alertBox.setAttribute("origin", "bottom");
    }
  }

  alertBox.orient = (gOrigin & NS_ALERT_HORIZONTAL) ? "vertical" : "horizontal";

  sizeToContent();

  
  gCurrentSize = 1;

  
  if (gOrigin & NS_ALERT_HORIZONTAL)
  {
    gFinalSize = window.outerWidth;
    window.outerWidth = gCurrentSize;
  }
  else
  {
    gFinalSize = window.outerHeight;
    window.outerHeight = gCurrentSize;
  }

  
  var x = gOrigin & NS_ALERT_LEFT ? screen.availLeft :
          screen.availLeft + screen.availWidth - window.outerWidth;
  var y = gOrigin & NS_ALERT_TOP ? screen.availTop :
          screen.availTop + screen.availHeight - window.outerHeight;

  
  if (gOrigin & NS_ALERT_HORIZONTAL)
    y += gOrigin & NS_ALERT_TOP ? 10 : -10;
  else
    x += gOrigin & NS_ALERT_LEFT ? 10 : -10;

  window.moveTo(x, y);

  setTimeout(animateAlert, gSlideTime);
}

function animate(step)
{
  gCurrentSize += step;

  if (gFinalSize < gCurrentSize)
    gCurrentSize = gFinalSize;

  if (gOrigin & NS_ALERT_HORIZONTAL)
  {
    if (!(gOrigin & NS_ALERT_LEFT))
      window.screenX -= step;
    window.outerWidth = gCurrentSize;
  }
  else
  {
    if (!(gOrigin & NS_ALERT_TOP))
      window.screenY -= step;
    window.outerHeight = gCurrentSize;
  }
}

function animateAlert()
{
  if (gCurrentSize < gFinalSize)
  {
    if (gDisableSlideEffect)
      animate(gFinalSize); 
    else
      animate(gSlideIncrement);
    setTimeout(animateAlert, gSlideTime);
  }
  else
    setTimeout(animateCloseAlert, gOpenTime);  
}

function animateCloseAlert()
{
  if (gCurrentSize > 1 && !gDisableSlideEffect)
  {
    animate(-gSlideIncrement);
    setTimeout(animateCloseAlert, gSlideTime);
  }
  else
    closeAlert();
}

function closeAlert() {
  if (gAlertListener)
    gAlertListener.observe(null, "alertfinished", gAlertCookie); 
  window.close(); 
}

function onAlertClick()
{
  if (gAlertListener && gAlertTextClickable)
    gAlertListener.observe(null, "alertclickcallback", gAlertCookie);
  closeAlert();
}

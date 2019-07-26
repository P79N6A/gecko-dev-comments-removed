





Components.utils.import("resource://gre/modules/Services.jsm");


const NS_ALERT_HORIZONTAL = 1;
const NS_ALERT_LEFT = 2;
const NS_ALERT_TOP = 4;

var gOrigin = 0; 
 
var gAlertListener = null;
var gAlertTextClickable = false;
var gAlertCookie = "";

function prefillAlertInfo() {
  
  
  
  
  
  
  
  

  switch (window.arguments.length) {
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
      document.getElementById('alertTextLabel').textContent = window.arguments[2];
    case 2:
      document.getElementById('alertTitleLabel').setAttribute('value', window.arguments[1]);
    case 1:
      document.getElementById('alertImage').setAttribute('src', window.arguments[0]);
    case 0:
      break;
  }
}

function onAlertLoad() {
  const ALERT_DURATION_IMMEDIATE = 4000;
  let alertTextBox = document.getElementById("alertTextBox");
  let alertImageBox = document.getElementById("alertImageBox");
  alertImageBox.style.minHeight = alertTextBox.scrollHeight + "px";

  sizeToContent();

  
  let x = gOrigin & NS_ALERT_LEFT ? screen.availLeft :
          screen.availLeft + screen.availWidth - window.outerWidth;
  let y = gOrigin & NS_ALERT_TOP ? screen.availTop :
          screen.availTop + screen.availHeight - window.outerHeight;

  
  y += gOrigin & NS_ALERT_TOP ? 10 : -10;
  x += gOrigin & NS_ALERT_LEFT ? 10 : -10;

  window.moveTo(x, y);

  if (Services.prefs.getBoolPref("alerts.disableSlidingEffect")) {
    setTimeout(closeAlert, ALERT_DURATION_IMMEDIATE);
    return;
  }

  let alertBox = document.getElementById("alertBox");
  alertBox.addEventListener("animationend", function hideAlert(event) {
    if (event.animationName == "alert-animation") {
      alertBox.removeEventListener("animationend", hideAlert, false);
      closeAlert();
    }
  }, false);
  alertBox.setAttribute("animate", true);
}

function closeAlert() {
  if (gAlertListener)
    gAlertListener.observe(null, "alertfinished", gAlertCookie);
  window.close();
}

function onAlertClick() {
  if (gAlertListener && gAlertTextClickable)
    gAlertListener.observe(null, "alertclickcallback", gAlertCookie);
  closeAlert();
}

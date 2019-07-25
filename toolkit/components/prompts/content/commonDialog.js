








































const Ci = Components.interfaces;
const Cr = Components.results;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

let gArgs = window.arguments[0].QueryInterface(Ci.nsIWritablePropertyBag2)
                               .QueryInterface(Ci.nsIWritablePropertyBag);

let promptType, numButtons, iconClass, soundID, hasInputField = true;


function earlyInit() {
    
    
    

    promptType = gArgs.getProperty("promptType");

    switch (promptType) {
      case "alert":
      case "alertCheck":
        hasInputField = false;
        numButtons    = 1;
        iconClass     = "alert-icon";
        soundID       = Ci.nsISound.EVENT_ALERT_DIALOG_OPEN;
        break;
      case "confirmCheck":
      case "confirm":
        hasInputField = false;
        numButtons    = 2;
        iconClass     = "question-icon";
        soundID       = Ci.nsISound.EVENT_CONFIRM_DIALOG_OPEN;
        break;
      case "confirmEx":
        numButtons = 0;
        if (gArgs.hasKey("button0Label"))
            numButtons++;
        if (gArgs.hasKey("button1Label"))
            numButtons++;
        if (gArgs.hasKey("button2Label"))
            numButtons++;
        if (gArgs.hasKey("button3Label"))
            numButtons++;
        if (numButtons == 0)
            throw "A dialog with no buttons? Can not haz.";
        hasInputField = false;
        iconClass     = "question-icon";
        soundID       = Ci.nsISound.EVENT_CONFIRM_DIALOG_OPEN;
        break;
      case "prompt":
        numButtons = 2;
        iconClass  = "question-icon";
        soundID    = Ci.nsISound.EVENT_PROMPT_DIALOG_OPEN;
        initTextbox("login", gArgs.getProperty("value"));
        
        document.getElementById("loginLabel").setAttribute("value", "");
        break;
      case "promptUserAndPass":
        numButtons = 2;
        iconClass  = "authentication-icon question-icon";
        soundID    = Ci.nsISound.EVENT_PROMPT_DIALOG_OPEN;
        initTextbox("login",     gArgs.getProperty("user"));
        initTextbox("password1", gArgs.getProperty("pass"));
        break;
      case "promptPassword":
        numButtons = 2;
        iconClass  = "authentication-icon question-icon";
        soundID    = Ci.nsISound.EVENT_PROMPT_DIALOG_OPEN;
        initTextbox("password1", gArgs.getProperty("pass"));
        
        document.getElementById("password1Label").setAttribute("value", "");
        break;
      default:
        Cu.reportError("commonDialog opened for unknown type: " + promptType);
        window.close();
    }
}

function initTextbox(aName, aValue) {
    document.getElementById(aName + "Container").hidden = false;
    document.getElementById(aName + "Textbox").setAttribute("value", aValue);
}

function setLabelForNode(aNode, aLabel, aIsLabelFlag) {
    
    
    
    
    
    

    
    
    var accessKey = null;
    if (/ *\(\&([^&])\)(:)?$/.test(aLabel)) {
        aLabel = RegExp.leftContext + RegExp.$2;
        accessKey = RegExp.$1;
    } else if (/^(.*[^&])?\&(([^&]).*$)/.test(aLabel)) {
        aLabel = RegExp.$1 + RegExp.$2;
        accessKey = RegExp.$3;
    }

    
    aLabel = aLabel.replace(/\&\&/g, "&");
    if (aIsLabelFlag) {    
        aNode.setAttribute("value", aLabel);
    } else {    
        aNode.label = aLabel;
    }

    
    
    if (accessKey)
        aNode.accessKey = accessKey;
}

function softkbObserver(subject, topic, data) {
    let rect = JSON.parse(data);
    if (rect) {
        let height = rect.bottom - rect.top;
        let width  = rect.right - rect.left;
        let top    = (rect.top + (height - window.innerHeight) / 2);
        let left   = (rect.left + (width - window.innerWidth) / 2);
        window.moveTo(left, top);
    }
}

function commonDialogOnLoad() {
    
    document.getElementById("filler").maxWidth = screen.availWidth;

    
    let title = gArgs.getProperty("title");
#ifdef XP_MACOSX
    document.getElementById("info.title").appendChild(document.createTextNode(title));
#else
    document.title = title;
#endif

    Services.obs.addObserver(softkbObserver, "softkb-change", false);

    
    let dialog = document.documentElement;
    switch (numButtons) {
      case 1:
        dialog.getButton("cancel").hidden = true;
        break;
      case 4:
        dialog.getButton("extra2").hidden = false;
      case 3:
        dialog.getButton("extra1").hidden = false;
    }

    
    switch (numButtons) {
      case 4:
        setLabelForNode(document.documentElement.getButton("extra2"), gArgs.getProperty("button3Label"));
        
      case 3:
        setLabelForNode(document.documentElement.getButton("extra1"), gArgs.getProperty("button2Label"));
        
      default:
      case 2:
        if (gArgs.hasKey("button1Label"))
            setLabelForNode(document.documentElement.getButton("cancel"), gArgs.getProperty("button1Label"));
        
      case 1:
        if (gArgs.hasKey("button0Label"))
            setLabelForNode(document.documentElement.getButton("accept"), gArgs.getProperty("button0Label"));
        break;
    }

    
    
    let croppedMessage = gArgs.getProperty("text").substr(0, 10000);
    document.getElementById("info.body").appendChild(document.createTextNode(croppedMessage));

    if (gArgs.hasKey("checkLabel")) {
        let label = gArgs.getProperty("checkLabel")
        
        if (label) {
            document.getElementById("checkboxContainer").hidden = false;
            let checkboxElement = document.getElementById("checkbox");
            setLabelForNode(checkboxElement, label);
            checkboxElement.checked = gArgs.getProperty("checked");
        }
    }

    
    document.getElementById("info.icon").className += " " + iconClass;

    
    gArgs.setProperty("ok", false);
    gArgs.setProperty("buttonNumClicked", 1);


    
    
    if (!hasInputField) {
        let dlgButtons = ['accept', 'cancel', 'extra1', 'extra2'];

        
        let b = 0;
        if (gArgs.hasKey("defaultButtonNum"))
            b = gArgs.getProperty("defaultButtonNum");
        let dButton = dlgButtons[b];
        
        document.documentElement.defaultButton = dButton;
#ifndef XP_MACOSX
        document.documentElement.getButton(dButton).focus();
#endif
    } else {
        if (promptType == "promptPassword")
            document.getElementById("password1Textbox").select();
        else
            document.getElementById("loginTextbox").select();
    }

    if (gArgs.hasKey("enableDelay") && gArgs.getProperty("enableDelay")) {
        let delayInterval = Services.prefs.getIntPref("security.dialog_enable_delay");

        document.documentElement.getButton("accept").disabled = true;
        document.documentElement.getButton("extra1").disabled = true;
        document.documentElement.getButton("extra2").disabled = true;

        setTimeout(commonDialogReenableButtons, delayInterval);

        addEventListener("blur", commonDialogBlur, false);
        addEventListener("focus", commonDialogFocus, false);
    }

    window.getAttention();

    
    try {
        if (soundID) {
            Cc["@mozilla.org/sound;1"].
            createInstance(Ci.nsISound).
            playEventSound(soundID);
        }
    } catch (e) { }

    Services.obs.notifyObservers(window, "common-dialog-loaded", null);
}

function commonDialogOnUnload() {
    Services.obs.removeObserver(softkbObserver, "softkb-change");
}


var gDelayExpired = false;
var gBlurred = false;

function commonDialogBlur(aEvent) {
    if (aEvent.target != document)
        return;
    gBlurred = true;
    document.documentElement.getButton("accept").disabled = true;
    document.documentElement.getButton("extra1").disabled = true;
    document.documentElement.getButton("extra2").disabled = true;
}

function commonDialogFocus(aEvent) {
    if (aEvent.target != document)
        return;
    gBlurred = false;
    
    
    if (gDelayExpired) {
        let script;
        script  = "document.documentElement.getButton('accept').disabled = false; ";
        script += "document.documentElement.getButton('extra1').disabled = false; ";
        script += "document.documentElement.getButton('extra2').disabled = false;";
        setTimeout(script, 250);
    }
}

function commonDialogReenableButtons() {
    
    if (!gBlurred) {
        document.documentElement.getButton("accept").disabled = false;
        document.documentElement.getButton("extra1").disabled = false;
        document.documentElement.getButton("extra2").disabled = false;
    }
    gDelayExpired = true;
}

function onCheckboxClick(aCheckboxElement) {
    gArgs.setProperty("checked", aCheckboxElement.checked);
}

function commonDialogOnAccept() {
    gArgs.setProperty("ok", true);
    gArgs.setProperty("buttonNumClicked", 0);

    let username = document.getElementById("loginTextbox").value;
    let password = document.getElementById("password1Textbox").value;

    
    switch (promptType) {
      case "prompt":
        gArgs.setProperty("value", username);
        break;
      case "promptUserAndPass":
        gArgs.setProperty("user", username);
        gArgs.setProperty("pass", password);
        break;
      case "promptPassword":
        gArgs.setProperty("pass", password);
        break;
    }
}

function commonDialogOnExtra1() {
    
    gArgs.setProperty("buttonNumClicked", 2);
    window.close();
}

function commonDialogOnExtra2() {
    
    gArgs.setProperty("buttonNumClicked", 3);
    window.close();
}

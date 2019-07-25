





































const Ci = Components.interfaces;
const Cr = Components.results;
const Cc = Components.classes;
const Cu = Components.utils;

let gArgs, listBox;

function dialogOnLoad() {
    gArgs = window.arguments[0].QueryInterface(Ci.nsIWritablePropertyBag2)
                               .QueryInterface(Ci.nsIWritablePropertyBag);

    let promptType = gArgs.getProperty("promptType");
    if (promptType != "select") {
        Cu.reportError("selectDialog opened for unknown type: " + promptType);
        window.close();
    }

    
    gArgs.setProperty("ok", false);

    document.title = gArgs.getProperty("title");

    let text = gArgs.getProperty("text");
    document.getElementById("info.txt").setAttribute("value", text);

    let items = gArgs.getProperty("list");
    listBox = document.getElementById("list");

    for (let i = 0; i < items.length; i++) {
        let str = items[i];
        if (str == "")
            str = "<>";
        listBox.appendItem(str);
        listBox.getItemAtIndex(i).addEventListener("dblclick", dialogDoubleClick, false);
    }
    listBox.selectedIndex = 0;
    listBox.focus();

    
    window.sizeToContent();

    
    moveToAlertPosition();
    centerWindowOnScreen();

    
    try {
        Cc["@mozilla.org/sound;1"].
        createInstance(Ci.nsISound).
        playEventSound(Ci.nsISound.EVENT_SELECT_DIALOG_OPEN);
    } catch (e) { }
}

function dialogOK() {
    let selected = listBox.selectedIndex;
    gArgs.setProperty("selected", listBox.selectedIndex);
    gArgs.setProperty("ok", true);
    return true;
}

function dialogDoubleClick() {
    dialogOK();
    window.close();
}






function getNotificationBox(aWindow) {
    var chromeWin = aWindow
                        .QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIWebNavigation)
                        .QueryInterface(Ci.nsIDocShellTreeItem)
                        .rootTreeItem
                        .QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIDOMWindow)
                        .QueryInterface(Ci.nsIDOMChromeWindow);

    
    var notifyBox = chromeWin.getNotificationBox(aWindow);
    return notifyBox;
}






function getNotificationBar(aBox, aKind) {
    ok(true, "Looking for " + aKind + " notification bar");
    
    ok(aBox.allNotifications.length <= 1, "Checking for multiple notifications");
    return aBox.getNotificationWithValue(aKind);
}







function clickNotificationButton(aBar, aButtonIndex) {
    
    
    
    var button = aBar.getElementsByTagName("button").item(aButtonIndex);
    ok(button, "Got button " + aButtonIndex);
    button.doCommand();
}

const kRememberButton = 0;
const kNeverButton = 1;
const kNotNowButton = 2;

const kChangeButton = 0;
const kDontChangeButton = 1;

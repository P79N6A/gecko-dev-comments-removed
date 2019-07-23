




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







function clickNotificationButton(aBar, aButtonName) {
    
    
    
    var buttons = aBar.getElementsByTagName("button");
    var clicked = false;
    for (var i = 0; i < buttons.length; i++) {
        if (buttons[i].label == aButtonName) {
            buttons[i].click();
            clicked = true;
            break;
        }
    }

    ok(clicked, "Clicked \"" + aButtonName + "\" button"); 
}

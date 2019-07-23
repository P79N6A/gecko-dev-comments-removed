







































function ShowPaneObj(aPane, aLoader) {
  this.args = [aPane, aLoader];
}
ShowPaneObj.prototype = {
  args: null,
  method: function _openMenu(aPane, aLoader) {
    Stack.push(aLoader);
    aPane.parentNode.showPane(aPane);
  }
};




function ClosePreferences() {
  this.args = [];
};
ClosePreferences.prototype = {
  args: [],
  method: function() {
    var pWin = WM.getMostRecentWindow("Browser:Preferences");
    if (pWin) {
      pWin.close();
    }
    else {
      Components.utils.reportError("prefwindow not found, trying again");
    }
  }
};




function PrefPaneLoader() {
};
PrefPaneLoader.prototype = {
  _currentPane: null,
  args: [],
  method: function() {
    if (!this._currentPane) {
      
      Stack.push(this);
      return;
    }
    
    pane = this._currentPane.nextSibling;
    this._currentPane = null;
    while (pane) {
      if (pane.nodeName == 'prefpane') {
        Stack.push(new ShowPaneObj(pane, this));
        return;
      }
      pane = pane.nextSibling;
    }
  },
  
  handleEvent: function _hv(aEvent) {
    this._currentPane = aEvent.target;
  }
};









function RootPreference(aWindow) {
  this.args = [aWindow];
};
RootPreference.prototype = {
  args: [],
  method: function(aWindow) {
    WM.addListener(this);
    aWindow.openPreferences('paneMain');
  },
  
  onWindowTitleChange: function(aWindow, newTitle){},
  onOpenWindow: function(aWindow) {
    WM.removeListener(this);
    
    Stack._timer.initWithCallback({notify:function (aTimer) {Stack.pop();}},
                                  Stack._time, 1);
    Stack.push(new ClosePreferences());
    var ppl = new PrefPaneLoader();
    aWindow.docShell.QueryInterface(Ci.nsIInterfaceRequestor);
    var DOMwin = aWindow.docShell.getInterface(Ci.nsIDOMWindow);
    DOMwin.addEventListener('paneload', ppl, false);
    Stack.push(ppl);
  },
  onCloseWindow: function(aWindow){}
};

toRun.push(new TestDone('PREFERENCES'));
toRun.push(new RootPreference(wins[0]));
toRun.push(new TestStart('PREFERENCES'));

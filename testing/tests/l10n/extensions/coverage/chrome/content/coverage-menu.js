









































function CloseMenuObj(aPopup) {
  this.args = [aPopup];
}
CloseMenuObj.prototype = {
  args: null,
  method: function _openMenu(aPopup) {
    aPopup.hidePopup();
  }
};







function OpenMenuObj(aPopup) {
  this.args = [aPopup];
}
OpenMenuObj.prototype = {
  args: null,
  method: function _openMenu(aPopup) {
    aPopup.showPopup();
    Stack.push(new CloseMenuObj(aPopup));
    for (var i = aPopup.childNodes.length - 1; i>=0; --i) {
      var c = aPopup.childNodes[i];
      if (c.nodeName != 'menu' || c.childNodes.length == 0) {
        continue;
      }
      for each (var childpop in c.childNodes) {
        if (childpop.localName == "menupopup") {
          Stack.push(new OpenMenuObj(childpop));
        }
      }
    }
  }
};







function RootMenu(aWindow) {
  this._w = aWindow;
};
RootMenu.prototype = {
  args: [],
  method: function() {
    var mb = this._w.document.getElementById('main-menubar');
    for (var i = mb.childNodes.length - 1; i>=0; --i) {
      var m = mb.childNodes[i];
      Stack.push(new OpenMenuObj(m.firstChild));
    }
  },
  _w: null
};

toRun.push(new TestDone('MENUS'));
toRun.push(new RootMenu(wins[0]));
toRun.push(new TestStart('MENUS'));

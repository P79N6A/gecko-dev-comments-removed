




































var SBS = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
var bundle = SBS.createBundle("chrome://global/locale/appstrings.properties");




var loadAction = {
  isLoaded: false,
  args: [],
  method: function() {
    if (!this.isLoaded) {
      Stack.push(this);
    }
  }
};

function onFrameLoad(aEvent) {
  aEvent.stopPropagation();
  loadAction.isLoaded = true;
  return false;
};






function ErrPageLoad(aFrame, aProperty) {
  this.args = [aFrame, aProperty.key, aProperty.value];
};
ErrPageLoad.prototype = {
  args: null,
  method: function(aFrame, aErr, aDesc) {
    loadAction.isLoaded = false;
    var q = 'e=' + encodeURIComponent(aErr);
    q += '&u=' + encodeURIComponent('http://foo.bar');
    
    
    
    
    aDesc = aDesc.replace('%S', 'http://foo.bar');
    q += '&d=' + encodeURIComponent(aDesc);
    Stack.push(loadAction);
    aFrame.setAttribute('src', 'about:neterror?' + q);
  }
};







function RootNeterror() {
  this.args = [];
};
RootNeterror.prototype = {
  args: null,
  method: function(aIFrame) {
    var frame = document.getElementById('neterror-pane');
    var nErrors =
      [err for (err in SimpleGenerator(bundle.getSimpleEnumeration(),
                                       Ci.nsIPropertyElement))];
    nErrors.sort(function(aPl, aPr) {
                   return aPl.key > aPr.key ? 1 :
                     aPl.key < aPr.key ? -1 : 0;
                 });
    for each (err in nErrors) {
      Stack.push(new ErrPageLoad(frame, err));
    }
  }
};

toRun.push(new TestDone('NETERROR'));
toRun.push(new RootNeterror());
toRun.push(new TestStart('NETERROR'));

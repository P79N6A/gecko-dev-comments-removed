














































var viewer;



const kIMPORT_RULE = Components.interfaces.nsIDOMCSSRule.IMPORT_RULE;



window.addEventListener("load", BoxModelViewer_initialize, false);

function BoxModelViewer_initialize()
{
  viewer = new BoxModelViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}




function BoxModelViewer()
{
  this.mURL = window.location;
  this.mObsMan = new ObserverManager(this);
}

BoxModelViewer.prototype = 
{
  
  
  
  mSubject: null,
  mPane: null,
  
  
  

  get uid() { return "boxModel" },
  get pane() { return this.mPane },

  get subject() { return this.mSubject },
  set subject(aObject) 
  {
    this.mSubject = aObject;
    this.updateStatGroup();
    this.mObsMan.dispatchEvent("subjectChange", { subject: aObject });
  },

  initialize: function(aPane)
  {
    this.mPane = aPane;
    aPane.notifyViewerReady(this);
  },

  destroy: function()
  {
  },

  isCommandEnabled: function(aCommand)
  {
    return false;
  },
  
  getCommand: function(aCommand)
  {
    return null;
  },

  
  

  addObserver: function(aEvent, aObserver) { this.mObsMan.addObserver(aEvent, aObserver); },
  removeObserver: function(aEvent, aObserver) { this.mObsMan.removeObserver(aEvent, aObserver); },
  
  
  
  
  updateStatGroup: function()
  {
    var ml = document.getElementById("mlStats");
    this.showStatGroup(ml.value);
  },
  
  showStatGroup: function(aGroup)
  {
    if (aGroup == "position") {
      this.showPositionStats();
    } else if (aGroup == "dimension") {
      this.showDimensionStats();
    } else if (aGroup == "margin") {
      this.showMarginStats();
    } else if (aGroup == "border") {
      this.showBorderStats();
    } else if (aGroup == "padding") {
      this.showPaddingStats();
    }    
  },
  
  showStatistic: function(aCol, aRow, aSide, aSize)
  {
    var label = document.getElementById("txR"+aRow+"C"+aCol+"Label");
    var val = document.getElementById("txR"+aRow+"C"+aCol+"Value");
    label.setAttribute("value", aSide && aSide.length ? aSide + ":" : "");
    val.setAttribute("value", aSize);
  },
  
  showPositionStats: function()
  {
    if ("boxObject" in this.mSubject) { 
      var bx = this.mSubject.boxObject;
      this.showStatistic(1, 1, "x", bx.x);
      this.showStatistic(1, 2, "y", bx.y);
      this.showStatistic(2, 1, "screen x", bx.screenX);
      this.showStatistic(2, 2, "screen y", bx.screenY);
    } else { 
      this.showStatistic(1, 1, "x", this.mSubject.offsetLeft);
      this.showStatistic(1, 2, "y", this.mSubject.offsetTop);
      this.showStatistic(2, 1, "", "");
      this.showStatistic(2, 2, "", "");
    }
  },
  
  showDimensionStats: function()
  {
    if ("boxObject" in this.mSubject) { 
      var bx = this.mSubject.boxObject;
      this.showStatistic(1, 1, "box width", bx.width);
      this.showStatistic(1, 2, "box height", bx.height);
      this.showStatistic(2, 1, "content width", "");
      this.showStatistic(2, 2, "content height", "");
      this.showStatistic(3, 1, "", "");
      this.showStatistic(3, 2, "", "");
    } else { 
      this.showStatistic(1, 1, "box width", this.mSubject.offsetWidth);
      this.showStatistic(1, 2, "box height", this.mSubject.offsetHeight);
      this.showStatistic(2, 1, "content width", "");
      this.showStatistic(2, 2, "content height", "");
      this.showStatistic(3, 1, "", "");
      this.showStatistic(3, 2, "", "");
    }
  },

  getSubjectComputedStyle: function()
  {
    var view = this.mSubject.ownerDocument.defaultView;
    return view.getComputedStyle(this.mSubject, "");
  },

  showMarginStats: function()
  {
    var style = this.getSubjectComputedStyle();
    var data = [this.readMarginStyle(style, "top"), this.readMarginStyle(style, "right"), 
                this.readMarginStyle(style, "bottom"), this.readMarginStyle(style, "left")];
    this.showSideStats("margin", data);                
  },

  showBorderStats: function()
  {
    var style = this.getSubjectComputedStyle();
    var data = [this.readBorderStyle(style, "top"), this.readBorderStyle(style, "right"), 
                this.readBorderStyle(style, "bottom"), this.readBorderStyle(style, "left")];
    this.showSideStats("border", data);                
  },

  showPaddingStats: function()
  {
    var style = this.getSubjectComputedStyle();
    var data = [this.readPaddingStyle(style, "top"), this.readPaddingStyle(style, "right"), 
                this.readPaddingStyle(style, "bottom"), this.readPaddingStyle(style, "left")];
    this.showSideStats("padding", data);
  },

  showSideStats: function(aName, aData)
  {
    this.showStatistic(1, 1, aName+"-top", aData[0]);
    this.showStatistic(2, 1, aName+"-right", aData[1]);
    this.showStatistic(1, 2, aName+"-bottom", aData[2]);
    this.showStatistic(2, 2, aName+"-left", aData[3]);
    this.showStatistic(3, 1, "", "");
    this.showStatistic(3, 2, "", "");
  },
  
  readMarginStyle: function(aStyle, aSide)
  {
    return aStyle.getPropertyCSSValue("margin-"+aSide).cssText;
  },
  
  readPaddingStyle: function(aStyle, aSide)
  {
    return aStyle.getPropertyCSSValue("padding-"+aSide).cssText;
  },
  
  readBorderStyle: function(aStyle, aSide)
  {
    var style = aStyle.getPropertyCSSValue("border-"+aSide+"-style").cssText;
    if (!style || !style.length) {
      return "none";
    } else {
      return aStyle.getPropertyCSSValue("border-"+aSide+"-width").cssText + " " + 
             style + " " +
             aStyle.getPropertyCSSValue("border-"+aSide+"-color").cssText;
    }
  }
};

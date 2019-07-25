





































"use strict";

const EXPORTED_SYMBOLS = ["SplitView"];


const LANDSCAPE_MEDIA_QUERY = "(min-aspect-ratio: 5/3)";

const BINDING_USERDATA = "splitview-binding";















function SplitView(aRoot)
{
  this._root = aRoot;
  this._controller = aRoot.querySelector(".splitview-controller");
  this._nav = aRoot.querySelector(".splitview-nav");
  this._side = aRoot.querySelector(".splitview-side-details");
  this._activeSummary = null

  this._mql = aRoot.ownerDocument.defaultView.matchMedia(LANDSCAPE_MEDIA_QUERY);

  
  this._nav.addEventListener("keydown", function onKeyCatchAll(aEvent) {
    function getFocusedItemWithin(nav) {
      let node = nav.ownerDocument.activeElement;
      while (node && node.parentNode != nav) {
        node = node.parentNode;
      }
      return node;
    }

    
    if (aEvent.target.ownerDocument != this._nav.ownerDocument ||
        aEvent.target.tagName == "input" ||
        aEvent.target.tagName == "textbox" ||
        aEvent.target.tagName == "textarea" ||
        aEvent.target.classList.contains("textbox")) {
      return false;
    }

    
    let newFocusOrdinal;
    if (aEvent.keyCode == aEvent.DOM_VK_PAGE_UP ||
        aEvent.keyCode == aEvent.DOM_VK_HOME) {
      newFocusOrdinal = 0;
    } else if (aEvent.keyCode == aEvent.DOM_VK_PAGE_DOWN ||
               aEvent.keyCode == aEvent.DOM_VK_END) {
      newFocusOrdinal = this._nav.childNodes.length - 1;
    } else if (aEvent.keyCode == aEvent.DOM_VK_UP) {
      newFocusOrdinal = getFocusedItemWithin(this._nav).getAttribute("data-ordinal");
      newFocusOrdinal--;
    } else if (aEvent.keyCode == aEvent.DOM_VK_DOWN) {
      newFocusOrdinal = getFocusedItemWithin(this._nav).getAttribute("data-ordinal");
      newFocusOrdinal++;
    }
    if (newFocusOrdinal !== undefined) {
      aEvent.stopPropagation();
      let el = this.getSummaryElementByOrdinal(newFocusOrdinal);
      if (el) {
        el.focus();
      }
      return false;
    }
  }.bind(this), false);
}

SplitView.prototype = {
  




  get isLandscape() this._mql.matches,

  




  get rootElement() this._root,

  




  get activeSummary() this._activeSummary,

  




  set activeSummary(aSummary)
  {
    if (aSummary == this._activeSummary) {
      return;
    }

    if (this._activeSummary) {
      let binding = this._activeSummary.getUserData(BINDING_USERDATA);

      if (binding.onHide) {
        binding.onHide(this._activeSummary, binding._details, binding.data);
      }

      this._activeSummary.classList.remove("splitview-active");
      binding._details.classList.remove("splitview-active");
    }

    if (!aSummary) {
      return;
    }

    let binding = aSummary.getUserData(BINDING_USERDATA);
    aSummary.classList.add("splitview-active");
    binding._details.classList.add("splitview-active");

    this._activeSummary = aSummary;

    if (binding.onShow) {
      binding.onShow(aSummary, binding._details, binding.data);
    }
    aSummary.scrollIntoView();
  },

  



  get activeDetails()
  {
    let summary = this.activeSummary;
    return summary ? summary.getUserData(BINDING_USERDATA)._details : null;
  },

  







  getSummaryElementByOrdinal: function SEC_getSummaryElementByOrdinal(aOrdinal)
  {
    return this._nav.querySelector("* > li[data-ordinal='" + aOrdinal + "']");
  },

  























  appendItem: function ASV_appendItem(aSummary, aDetails, aOptions)
  {
    let binding = aOptions || {};

    binding._summary = aSummary;
    binding._details = aDetails;
    aSummary.setUserData(BINDING_USERDATA, binding, null);

    this._nav.appendChild(aSummary);

    aSummary.addEventListener("click", function onSummaryClick(aEvent) {
      aEvent.stopPropagation();
      this.activeSummary = aSummary;
    }.bind(this), false);

    this._side.appendChild(aDetails);

    if (binding.onCreate) {
      
      this._root.ownerDocument.defaultView.setTimeout(function () {
        binding.onCreate(aSummary, aDetails, binding.data);
      }, 0);
    }
  },

  














  appendTemplatedItem: function ASV_appendTemplatedItem(aName, aOptions)
  {
    aOptions = aOptions || {};
    let summary = this._root.querySelector("#splitview-tpl-summary-" + aName);
    let details = this._root.querySelector("#splitview-tpl-details-" + aName);

    summary = summary.cloneNode(true);
    summary.id = "";
    if (aOptions.ordinal !== undefined) { 
      summary.style.MozBoxOrdinalGroup = aOptions.ordinal;
      summary.setAttribute("data-ordinal", aOptions.ordinal);
    }
    details = details.cloneNode(true);
    details.id = "";

    this.appendItem(summary, details, aOptions);
    return {summary: summary, details: details};
  },

  





  removeItem: function ASV_removeItem(aSummary)
  {
    if (aSummary == this._activeSummary) {
      this.activeSummary = null;
    }

    let binding = aSummary.getUserData(BINDING_USERDATA);
    aSummary.parentNode.removeChild(aSummary);
    binding._details.parentNode.removeChild(binding._details);

    if (binding.onDestroy) {
      binding.onDestroy(aSummary, binding._details, binding.data);
    }
  },

  


  removeAll: function ASV_removeAll()
  {
    while (this._nav.hasChildNodes()) {
      this.removeItem(this._nav.firstChild);
    }
  },

  









  setItemClassName: function ASV_setItemClassName(aSummary, aClassName)
  {
    let binding = aSummary.getUserData(BINDING_USERDATA);
    let viewSpecific;

    viewSpecific = aSummary.className.match(/(splitview\-[\w-]+)/g);
    viewSpecific = viewSpecific ? viewSpecific.join(" ") : "";
    aSummary.className = viewSpecific + " " + aClassName;

    viewSpecific = binding._details.className.match(/(splitview\-[\w-]+)/g);
    viewSpecific = viewSpecific ? viewSpecific.join(" ") : "";
    binding._details.className = viewSpecific + " " + aClassName;
  },
};

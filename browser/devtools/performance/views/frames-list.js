


"use strict";

const HTML_NS = "http://www.w3.org/1999/xhtml";
const PERCENTAGE_UNITS = L10N.getStr("table.percentage");





let FramesListView = {

  
  _selectedItem: null,

  


  initialize: function ({ container }) {
    this._onFrameListClick = this._onFrameListClick.bind(this);

    this.container = container;
    this.list = document.createElementNS(HTML_NS, "ul");
    this.list.setAttribute("class", "frames-list");
    this.list.addEventListener("click", this._onFrameListClick, false);

    this.container.appendChild(this.list);
  },

  


  destroy: function () {
    this.list.removeEventListener("click", this._onFrameListClick, false);
    this.container.innerHTML = "";
    this.container = this.list = null;
  },

  




  setCurrentThread: function (threadNode) {
    this.threadNode = threadNode;
  },

  



  render: function () {
    this.list.innerHTML = "";

    if (!this.threadNode) {
      return;
    }

    let totalSamples = this.threadNode.samples;
    let sortedFrames = this.threadNode.calls.sort((a, b) => a.youngestFrameSamples < b.youngestFrameSamples ? 1 : -1);
    for (let frame of sortedFrames) {
      if (!frame.hasOptimizations()) {
        continue;
      }
      let info = frame.getInfo();
      let el = document.createElementNS(HTML_NS, "li");
      let percentage = frame.youngestFrameSamples / totalSamples * 100;
      let percentageText = L10N.numberWithDecimals(percentage, 2) + PERCENTAGE_UNITS;
      let label = `(${percentageText}) ${info.functionName}`;
      el.textContent = label;
      el.setAttribute("tooltip", label);
      el.setAttribute("data-location", frame.location);
      this.list.appendChild(el);
    }
  },

  


  _onFrameListClick: function (e) {
    
    
    if (!this.threadNode || e.button !== 0) {
      return;
    }

    let target = e.target;
    let location = target.getAttribute("data-location");
    if (!location) {
      return;
    }

    for (let frame of this.threadNode.calls) {
      if (frame.location === location) {
        
        
        if (this._selectedItem) {
          this._selectedItem.classList.remove("selected");
        }
        this._selectedItem = target;
        target.classList.add("selected");
        console.log("Emitting select on", this, frame);
        this.emit("select", frame);
        break;
      }
    }
  },

  toString: () => "[object FramesListView]"
};

EventEmitter.decorate(FramesListView);

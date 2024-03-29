


"use strict";

const WATERFALL_RESIZE_EVENTS_DRAIN = 100; 
const MARKER_DETAILS_WIDTH = 200;




let WaterfallView = Heritage.extend(DetailsSubview, {

  observedPrefs: [
    "hidden-markers"
  ],

  rerenderPrefs: [
    "hidden-markers"
  ],

  rangeChangeDebounceTime: 75, 

  


  initialize: function () {
    DetailsSubview.initialize.call(this);

    this._cache = new WeakMap();

    this._onMarkerSelected = this._onMarkerSelected.bind(this);
    this._onResize = this._onResize.bind(this);
    this._onViewSource = this._onViewSource.bind(this);
    this._hiddenMarkers = PerformanceController.getPref("hidden-markers");

    this.headerContainer = $("#waterfall-header");
    this.breakdownContainer = $("#waterfall-breakdown");
    this.detailsContainer = $("#waterfall-details");
    this.detailsSplitter = $("#waterfall-view > splitter");

    this.details = new MarkerDetails($("#waterfall-details"), $("#waterfall-view > splitter"));
    this.details.hidden = true;

    this.details.on("resize", this._onResize);
    this.details.on("view-source", this._onViewSource);
    window.addEventListener("resize", this._onResize);

    
    this.details.width = MARKER_DETAILS_WIDTH;
  },

  


  destroy: function () {
    DetailsSubview.destroy.call(this);

    this._cache = null;

    this.details.off("resize", this._onResize);
    this.details.off("view-source", this._onViewSource);
    window.removeEventListener("resize", this._onResize);
  },

  





  render: function(interval={}) {
    let recording = PerformanceController.getCurrentRecording();
    let startTime = interval.startTime || 0;
    let endTime = interval.endTime || recording.getDuration();
    let markers = recording.getMarkers();
    let rootMarkerNode = this._prepareWaterfallTree(markers);

    this._populateWaterfallTree(rootMarkerNode, { startTime, endTime });
    this.emit(EVENTS.WATERFALL_RENDERED);
  },

  



  _onMarkerSelected: function (event, marker) {
    let recording = PerformanceController.getCurrentRecording();
    let frames = recording.getFrames();

    if (event === "selected") {
      this.details.render({ toolbox: gToolbox, marker, frames });
      this.details.hidden = false;
      this._lastSelected = marker;
    }
    if (event === "unselected") {
      this.details.empty();
    }
  },

  


  _onResize: function () {
    setNamedTimeout("waterfall-resize", WATERFALL_RESIZE_EVENTS_DRAIN, () => {
      this._markersRoot.recalculateBounds();
      this.render(OverviewView.getTimeInterval());
    });
  },

  


  _onObservedPrefChange: function(_, prefName) {
    this._hiddenMarkers = PerformanceController.getPref("hidden-markers");

    
    
    this._cache = new WeakMap();
  },

  


  _onViewSource: function (_, file, line) {
    gToolbox.viewSourceInDebugger(file, line);
  },

  



  _prepareWaterfallTree: function(markers) {
    let cached = this._cache.get(markers);
    if (cached) {
      return cached;
    }

    let rootMarkerNode = WaterfallUtils.createParentNode({ name: "(root)" });

    WaterfallUtils.collapseMarkersIntoNode({
      rootNode: rootMarkerNode,
      markersList: markers,
      filter: this._hiddenMarkers
    });

    this._cache.set(markers, rootMarkerNode);
    return rootMarkerNode;
  },

  


  _populateWaterfallTree: function(rootMarkerNode, interval) {
    let root = new MarkerView({
      marker: rootMarkerNode,
      
      hidden: true,
      
      autoExpandDepth: 0
    });

    let header = new WaterfallHeader(root);

    this._markersRoot = root;
    this._waterfallHeader = header;

    root.filter = this._hiddenMarkers;
    root.interval = interval;
    root.on("selected", this._onMarkerSelected);
    root.on("unselected", this._onMarkerSelected);

    this.breakdownContainer.innerHTML = "";
    root.attachTo(this.breakdownContainer);

    this.headerContainer.innerHTML = "";
    header.attachTo(this.headerContainer);

    
    
    if (this._lastSelected) {
      let item = root.find(i => i.marker === this._lastSelected);
      if (item) {
        item.focus();
      }
    }
  },

  toString: () => "[object WaterfallView]"
});

EventEmitter.decorate(WaterfallView);

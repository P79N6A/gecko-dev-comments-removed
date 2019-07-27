


"use strict";


















const { Ci, Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");


loader.lazyRequireGetter(this, "events", "sdk/event/core");
loader.lazyRequireGetter(this, "Timers", "sdk/timers");
loader.lazyRequireGetter(this, "Task", "resource://gre/modules/Task.jsm", true);
loader.lazyRequireGetter(this, "Memory", "devtools/toolkit/shared/memory", true);
loader.lazyRequireGetter(this, "Framerate", "devtools/toolkit/shared/framerate", true);
loader.lazyRequireGetter(this, "StackFrameCache", "devtools/server/actors/utils/stack", true);
loader.lazyRequireGetter(this, "EventTarget", "sdk/event/target", true);




const DEFAULT_TIMELINE_DATA_PULL_TIMEOUT = 200; 




let Timeline = exports.Timeline = Class({
  extends: EventTarget,

  


  initialize: function (tabActor) {
    this.tabActor = tabActor;

    this._isRecording = false;
    this._stackFrames = null;
    this._memory = null;
    this._framerate = null;

    
    this._onWindowReady = this._onWindowReady.bind(this);
    this._onGarbageCollection = this._onGarbageCollection.bind(this);
    events.on(this.tabActor, "window-ready", this._onWindowReady);
  },

  


  destroy: function() {
    this.stop();

    events.off(this.tabActor, "window-ready", this._onWindowReady);
    this.tabActor = null;

    if (this._memory) {
      this._memory.destroy();
      this._memory = null;
    }

    if (this._framerate) {
      this._framerate.destroy();
      this._framerate = null;
    }
  },

  








  get docShells() {
    let originalDocShell;

    if (this.tabActor.isRootActor) {
      originalDocShell = this.tabActor.docShell;
    } else {
      originalDocShell = this.tabActor.originalDocShell;
    }

    let docShellsEnum = originalDocShell.getDocShellEnumerator(
      Ci.nsIDocShellTreeItem.typeAll,
      Ci.nsIDocShell.ENUMERATE_FORWARDS
    );

    let docShells = [];
    while (docShellsEnum.hasMoreElements()) {
      let docShell = docShellsEnum.getNext();
      docShells.push(docShell.QueryInterface(Ci.nsIDocShell));
    }

    return docShells;
  },

  



  _pullTimelineData: function() {
    if (!this._isRecording || !this.docShells.length) {
      return;
    }

    let endTime = this.docShells[0].now();
    let markers = [];

    for (let docShell of this.docShells) {
      markers.push(...docShell.popProfileTimelineMarkers());
    }

    
    
    
    
    
    
    for (let marker of markers) {
      if (marker.stack) {
        marker.stack = this._stackFrames.addFrame(Cu.waiveXrays(marker.stack));
      }
      if (marker.endStack) {
        marker.endStack = this._stackFrames.addFrame(Cu.waiveXrays(marker.endStack));
      }
    }

    let frames = this._stackFrames.makeEvent();
    if (frames) {
      events.emit(this, "frames", endTime, frames);
    }
    if (markers.length > 0) {
      events.emit(this, "markers", markers, endTime);
    }
    if (this._withMemory) {
      events.emit(this, "memory", endTime, this._memory.measure());
    }
    if (this._withTicks) {
      events.emit(this, "ticks", endTime, this._framerate.getPendingTicks());
    }

    this._dataPullTimeout = Timers.setTimeout(() => {
      this._pullTimelineData();
    }, DEFAULT_TIMELINE_DATA_PULL_TIMEOUT);
  },

  


  isRecording: function () {
    return this._isRecording;
  },

  










  start: Task.async(function *({ withMemory, withTicks }) {
    let startTime = this._startTime = this.docShells[0].now();

    if (this._isRecording) {
      return startTime;
    }

    this._isRecording = true;
    this._stackFrames = new StackFrameCache();
    this._stackFrames.initFrames();
    this._withMemory = withMemory;
    this._withTicks = withTicks;

    for (let docShell of this.docShells) {
      docShell.recordProfileTimelineMarkers = true;
    }

    this._memory = new Memory(this.tabActor, this._stackFrames);
    this._memory.attach();
    events.on(this._memory, "garbage-collection", this._onGarbageCollection);

    if (withTicks) {
      this._framerate = new Framerate(this.tabActor);
      this._framerate.startRecording();
    }

    this._pullTimelineData();
    return startTime;
  }),

  


  stop: Task.async(function *() {
    if (!this._isRecording) {
      return;
    }
    this._isRecording = false;
    this._stackFrames = null;

    events.off(this._memory, "garbage-collection", this._onGarbageCollection);
    this._memory.detach();

    if (this._framerate) {
      this._framerate.stopRecording();
      this._framerate = null;
    }

    for (let docShell of this.docShells) {
      docShell.recordProfileTimelineMarkers = false;
    }

    Timers.clearTimeout(this._dataPullTimeout);
    return this.docShells[0].now();
  }),

  



  _onWindowReady: function({ window }) {
    if (this._isRecording) {
      let docShell = window.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIWebNavigation)
                           .QueryInterface(Ci.nsIDocShell);
      docShell.recordProfileTimelineMarkers = true;
    }
  },

  








  _onGarbageCollection: function ({ collections, reason, nonincrementalReason }) {
    if (!this._isRecording || !this.docShells.length) {
      return;
    }

    let endTime = this.docShells[0].now();

    events.emit(this, "markers", collections.map(({ startTimestamp: start, endTimestamp: end }) => {
      return {
        name: "GarbageCollection",
        causeName: reason,
        nonincrementalReason: nonincrementalReason,
        
        start: start,
        end: end
      };
    }), endTime);
  },
});

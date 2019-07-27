


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");

loader.lazyRequireGetter(this, "PerformanceIO",
  "devtools/performance/io", true);
loader.lazyRequireGetter(this, "RecordingUtils",
  "devtools/performance/recording-utils", true);







const RecordingModel = function (options={}) {
  this._front = options.front;
  this._performance = options.performance;
  this._label = options.label || "";
};

RecordingModel.prototype = {
  
  _imported: false,
  _recording: false,
  _profilerStartTime: 0,
  _timelineStartTime: 0,

  
  _label: "",
  _duration: 0,
  _markers: null,
  _frames: null,
  _ticks: null,
  _memory: null,
  _profile: null,

  





  importRecording: Task.async(function *(file) {
    let recordingData = yield PerformanceIO.loadRecordingFromFile(file);

    this._imported = true;
    this._label = recordingData.label || "";
    this._duration = recordingData.duration;
    this._markers = recordingData.markers;
    this._frames = recordingData.frames;
    this._memory = recordingData.memory;
    this._ticks = recordingData.ticks;
    this._profile = recordingData.profile;
  }),

  





  exportRecording: Task.async(function *(file) {
    let recordingData = this.getAllData();
    yield PerformanceIO.saveRecordingToFile(recordingData, file);
  }),

  






  startRecording: Task.async(function *(options = {}) {
    
    
    
    
    this._localStartTime = this._performance.now();

    let info = yield this._front.startRecording(options);
    this._profilerStartTime = info.profilerStartTime;
    this._timelineStartTime = info.timelineStartTime;
    this._recording = true;

    this._markers = [];
    this._frames = [];
    this._memory = [];
    this._ticks = [];
  }),

  


  stopRecording: Task.async(function *() {
    let info = yield this._front.stopRecording();
    this._profile = info.profile;
    this._duration = info.profilerEndTime - this._profilerStartTime;
    this._recording = false;

    
    
    
    RecordingUtils.filterSamples(this._profile, this._profilerStartTime);
    RecordingUtils.offsetSampleTimes(this._profile, this._profilerStartTime);

    
    
    this._markers = this._markers.sort((a, b) => (a.start > b.start));
  }),

  



  getLabel: function () {
    return this._label;
  },

  



  getDuration: function () {
    
    
    
    if (this._recording) {
      return this._performance.now() - this._localStartTime;
    } else {
      return this._duration;
    }
  },

  



  getMarkers: function() {
    return this._markers;
  },

  



  getFrames: function() {
    return this._frames;
  },

  



  getMemory: function() {
    return this._memory;
  },

  



  getTicks: function() {
    return this._ticks;
  },

  



  getProfile: function() {
    return this._profile;
  },

  


  getAllData: function() {
    let label = this.getLabel();
    let duration = this.getDuration();
    let markers = this.getMarkers();
    let frames = this.getFrames();
    let memory = this.getMemory();
    let ticks = this.getTicks();
    let profile = this.getProfile();
    return { label, duration, markers, frames, memory, ticks, profile };
  },

  



  isRecording: function () {
    return this._recording;
  },

  


  addTimelineData: function (eventName, ...data) {
    
    
    if (!this._recording) {
      return;
    }

    switch (eventName) {
      
      
      case "markers":
        let [markers] = data;
        RecordingUtils.offsetMarkerTimes(markers, this._timelineStartTime);
        Array.prototype.push.apply(this._markers, markers);
        break;

      
      case "frames":
        let [, frames] = data;
        Array.prototype.push.apply(this._frames, frames);
        break;

      
      
      case "memory":
        let [currentTime, measurement] = data;
        this._memory.push({
          delta: currentTime - this._timelineStartTime,
          value: measurement.total / 1024 / 1024
        });
        break;

      
      case "ticks":
        let [, timestamps] = data;
        this._ticks = timestamps;
        break;
    }
  }
};

exports.RecordingModel = RecordingModel;




"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");
const { Task } = require("resource://gre/modules/Task.jsm");

loader.lazyRequireGetter(this, "PerformanceIO",
  "devtools/performance/io", true);
loader.lazyRequireGetter(this, "RecordingUtils",
  "devtools/performance/recording-utils", true);







const RecordingModel = function (options={}) {
  this._label = options.label || "";
  this._console = options.console || false;

  this._configuration = {
    withTicks: options.withTicks || false,
    withMemory: options.withMemory || false,
    withAllocations: options.withAllocations || false,
    allocationsSampleProbability: options.allocationsSampleProbability || 0,
    allocationsMaxLogLength: options.allocationsMaxLogLength || 0,
    bufferSize: options.bufferSize || 0,
    sampleFrequency: options.sampleFrequency || 1
  };
};

RecordingModel.prototype = {
  
  _console: false,
  _imported: false,
  _recording: false,
  _profilerStartTime: 0,
  _timelineStartTime: 0,
  _memoryStartTime: 0,
  _configuration: {},

  
  _label: "",
  _duration: 0,
  _markers: null,
  _frames: null,
  _memory: null,
  _ticks: null,
  _allocations: null,
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
    this._allocations = recordingData.allocations;
    this._profile = recordingData.profile;
  }),

  





  exportRecording: Task.async(function *(file) {
    let recordingData = this.getAllData();
    yield PerformanceIO.saveRecordingToFile(recordingData, file);
  }),

  



  populate: function (info) {
    
    
    
    
    this._localStartTime = Date.now()

    this._profilerStartTime = info.profilerStartTime;
    this._timelineStartTime = info.timelineStartTime;
    this._memoryStartTime = info.memoryStartTime;
    this._recording = true;

    this._markers = [];
    this._frames = [];
    this._memory = [];
    this._ticks = [];
    this._allocations = { sites: [], timestamps: [], frames: [], counts: [] };
  },

  



  _onStopRecording: Task.async(function *(info) {
    this._profile = info.profile;
    this._duration = info.profilerEndTime - this._profilerStartTime;
    this._recording = false;

    
    
    
    
    RecordingUtils.offsetSampleTimes(this._profile, this._profilerStartTime);

    
    
    this._markers = this._markers.sort((a, b) => (a.start > b.start));
  }),

  



  getProfilerStartTime: function () {
    return this._profilerStartTime;
  },

  



  getLabel: function () {
    return this._label;
  },

  



  getDuration: function () {
    
    
    
    if (this._recording) {
      return Date.now() - this._localStartTime;
    } else {
      return this._duration;
    }
  },

  




  getConfiguration: function () {
    return this._configuration;
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

  



  getAllocations: function() {
    return this._allocations;
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
    let allocations = this.getAllocations();
    let profile = this.getProfile();
    return { label, duration, markers, frames, memory, ticks, allocations, profile };
  },

  



  isImported: function () {
    return this._imported;
  },

  



  isConsole: function () {
    return this._console;
  },

  



  isRecording: function () {
    return this._recording;
  },

  


  addTimelineData: function (eventName, ...data) {
    
    
    if (!this._recording) {
      return;
    }

    switch (eventName) {
      
      
      case "markers": {
        let [markers] = data;
        RecordingUtils.offsetMarkerTimes(markers, this._timelineStartTime);
        Array.prototype.push.apply(this._markers, markers);
        break;
      }
      
      case "frames": {
        let [, frames] = data;
        Array.prototype.push.apply(this._frames, frames);
        break;
      }
      
      
      case "memory": {
        let [currentTime, measurement] = data;
        this._memory.push({
          delta: currentTime - this._timelineStartTime,
          value: measurement.total / 1024 / 1024
        });
        break;
      }
      
      case "ticks": {
        let [, timestamps] = data;
        this._ticks = timestamps;
        break;
      }
      
      
      
      case "allocations": {
        let [{ sites, timestamps, frames, counts }] = data;
        let timeOffset = this._memoryStartTime * 1000;
        let timeScale = 1000;
        RecordingUtils.offsetAndScaleTimestamps(timestamps, timeOffset, timeScale);
        Array.prototype.push.apply(this._allocations.sites, sites);
        Array.prototype.push.apply(this._allocations.timestamps, timestamps);
        Array.prototype.push.apply(this._allocations.frames, frames);
        Array.prototype.push.apply(this._allocations.counts, counts);
        break;
      }
    }
  }
};

exports.RecordingModel = RecordingModel;

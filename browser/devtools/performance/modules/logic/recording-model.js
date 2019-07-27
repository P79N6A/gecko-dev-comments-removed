


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");
const { Task } = require("resource://gre/modules/Task.jsm");

loader.lazyRequireGetter(this, "PerformanceIO",
  "devtools/performance/io", true);
loader.lazyRequireGetter(this, "RecordingUtils",
  "devtools/performance/recording-utils");






const RecordingModel = function (options={}) {
  this._label = options.label || "";
  this._console = options.console || false;

  this._configuration = {
    withMarkers: options.withMarkers || false,
    withTicks: options.withTicks || false,
    withMemory: options.withMemory || false,
    withAllocations: options.withAllocations || false,
    withJITOptimizations: options.withJITOptimizations || false,
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
  _completed: false,
  _profilerStartTime: 0,
  _timelineStartTime: 0,
  _memoryStartTime: 0,
  _configuration: {},
  _originalBufferStatus: null,
  _bufferPercent: null,

  
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
    this._configuration = recordingData.configuration || {};
  }),

  





  exportRecording: Task.async(function *(file) {
    let recordingData = this.getAllData();
    yield PerformanceIO.saveRecordingToFile(recordingData, file);
  }),

  



  _populate: function (info) {
    
    
    
    
    this._localStartTime = Date.now();

    this._profilerStartTime = info.profilerStartTime;
    this._timelineStartTime = info.timelineStartTime;
    this._memoryStartTime = info.memoryStartTime;
    this._originalBufferStatus = {
      position: info.position,
      totalSize: info.totalSize,
      generation: info.generation
    };
    
    this._bufferPercent = info.position !== void 0 ? 0 : null;

    this._recording = true;

    this._markers = [];
    this._frames = [];
    this._memory = [];
    this._ticks = [];
    this._allocations = { sites: [], timestamps: [], frames: [], counts: [] };
  },

  





  _onStoppingRecording: function (endTime) {
    this._duration = endTime - this._localStartTime;
    this._recording = false;
  },

  



  _onStopRecording: Task.async(function *({ profilerEndTime, profile }) {
    
    
    this._duration = profilerEndTime - this._profilerStartTime;
    this._profile = profile;
    this._completed = true;

    
    
    
    
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
    let configuration = this.getConfiguration();
    return { label, duration, markers, frames, memory, ticks, allocations, profile, configuration };
  },

  



  isImported: function () {
    return this._imported;
  },

  



  isConsole: function () {
    return this._console;
  },

  





  isCompleted: function () {
    return this._completed || this.isImported();
  },

  





  isRecording: function () {
    return this._recording;
  },

  



  getBufferUsage: function () {
    return this.isRecording() ? this._bufferPercent : null;
  },

  


  _addBufferStatusData: function (bufferStatus) {
    
    
    
    if (!bufferStatus || !this.isRecording()) {
      return;
    }
    let { position: currentPosition, totalSize, generation: currentGeneration } = bufferStatus;
    let { position: origPosition, generation: origGeneration } = this._originalBufferStatus;

    let normalizedCurrent = (totalSize * (currentGeneration - origGeneration)) + currentPosition;
    let percent = (normalizedCurrent - origPosition) / totalSize;
    this._bufferPercent = percent > 1 ? 1 : percent;
  },

  


  _addTimelineData: function (eventName, ...data) {
    
    
    if (!this.isRecording()) {
      return;
    }

    let config = this.getConfiguration();

    switch (eventName) {
      
      
      case "markers": {
        if (!config.withMarkers) { break; }
        let [markers] = data;
        RecordingUtils.offsetMarkerTimes(markers, this._timelineStartTime);
        pushAll(this._markers, markers);
        break;
      }
      
      case "frames": {
        if (!config.withMarkers) { break; }
        let [, frames] = data;
        pushAll(this._frames, frames);
        break;
      }
      
      
      case "memory": {
        if (!config.withMemory) { break; }
        let [currentTime, measurement] = data;
        this._memory.push({
          delta: currentTime - this._timelineStartTime,
          value: measurement.total / 1024 / 1024
        });
        break;
      }
      
      case "ticks": {
        if (!config.withTicks) { break; }
        let [, timestamps] = data;
        this._ticks = timestamps;
        break;
      }
      
      
      
      case "allocations": {
        if (!config.withAllocations) { break; }
        let [{ sites, timestamps, frames, counts }] = data;
        let timeOffset = this._memoryStartTime * 1000;
        let timeScale = 1000;
        RecordingUtils.offsetAndScaleTimestamps(timestamps, timeOffset, timeScale);
        pushAll(this._allocations.sites, sites);
        pushAll(this._allocations.timestamps, timestamps);
        pushAll(this._allocations.frames, frames);
        pushAll(this._allocations.counts, counts);
        break;
      }
    }
  },

  toString: () => "[object RecordingModel]"
};













function pushAll (dest, src) {
  let length = src.length;
  for (let i = 0; i < length; i++) {
    dest.push(src[i]);
  }
}

exports.RecordingModel = RecordingModel;

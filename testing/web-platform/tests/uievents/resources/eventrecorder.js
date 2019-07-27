






























































































(function EventRecorderScope(global) {
   "use strict";

   if (global.EventRecorder)
      return; 

   
   if (!global.WeakMap) {
      throw new Error("EventRecorder depends on WeakMap! Please polyfill for completeness to run in this user agent!");
   }

   
   var allRecords = [];
   var recording = false;
   var rawOrder = 1;
   var mergeTypesTruthMap = {}; 
   var eventsInScope = []; 
   var handlerMap = new WeakMap(); 

   
   var eventConstructorsNameMap = new WeakMap(); 
   var regex = /[A-Z][A-Za-z0-9]+Event$/;
   Object.getOwnPropertyNames(global).forEach(function (propName) {
        if (regex.test(propName))
         eventConstructorsNameMap.set(global[propName], propName);
   });
   var knownObjectsMap = eventConstructorsNameMap;

   Object.defineProperty(global, "EventRecorder", {
      writable: true,
      configurable: true,
      value: Object.create(null, {
         start: {
            enumerable: true, configurable: true, writable: true, value: function start() { recording = true; }
         },
         stop: {
            enumerable: true, configurable: true, writable: true, value: function stop() { recording = false; }
         },
         clearRecords: {
            enumerable: true, configurable: true, writable: true, value: function clearRecords() {
               rawOrder = 1;
               allRecords = [];
            }
         },
         getRecords: {
            enumerable: true, configurable: true, writable: true, value: function getRecords() { return allRecords; }
         },
         configure: {
            enumerable: true, configurable: true, writable: true, value: function configure(options) {
               if (allRecords.length > 0)
                  throw new Error("Wrong time to call me: EventRecorder.configure must only be called when no recorded events are present. Try 'clearRecords' first.");

               
               mergeTypesTruthMap = {};
               knownObjectsMap = eventConstructorsNameMap;

               if (!(options instanceof Object))
                  return;
               
               var sanitizedOptions = {};
               for (var x in options) {
                  sanitizedOptions[x] = options[x];
               }
               if (sanitizedOptions.mergeEventTypes && Array.isArray(sanitizedOptions.mergeEventTypes)) {
                  sanitizedOptions.mergeEventTypes.forEach(function (eventType) {
                     if (typeof eventType == "string")
                        mergeTypesTruthMap[eventType] = true;
                  });
               }
               if (sanitizedOptions.objectMap && (sanitizedOptions.objectMap instanceof Object)) {
                  for (var y in sanitizedOptions.objectMap) {
                     knownObjectsMap.set(sanitizedOptions.objectMap[y], y);
                  }
               }
            }
         }
      })
   });

   function EventRecord(rawEvent) {
      this.chronologicalOrder = rawOrder++;
      this.sequentialOccurrences = 1;
      this.nestedEvents = null; 
      this.interfaceType = knownObjectsMap.get(rawEvent.constructor);
      if (!this.interfaceType) 
         this.interfaceType = rawEvent.constructor.toString();
      this.event = new CloneObjectLike(rawEvent);
   }

   
   function CloneObjectLike(object) {
      for (var prop in object) {
         var val = object[prop];
         if (Array.isArray(val))
            this[prop] = CloneArray(val);
         else if (typeof val == "function")
            continue;
         else if ((typeof val == "object") && (val != null)) {
            this[prop] = knownObjectsMap.get(val);
            if (this[prop] === undefined)
               this[prop] = "UNKNOWN_OBJECT (" + val.toString() + ")";
         }
         else
            this[prop] = val;
      }
   }

   function CloneArray(array) {
      var dup = [];
      for (var i = 0, len = array.length; i < len; i++) {
         var val = array[i]
         if (typeof val == "undefined")
            throw new Error("Ugg. Sparce arrays are not supported. Sorry!");
         else if (Array.isArray(val))
            dup[i] = "UNKNOWN_ARRAY";
         else if (typeof val == "function")
            dup[i] = "UNKNOWN_FUNCTION";
         else if ((typeof val == "object") && (val != null)) {
            dup[i] = knownObjectsMap.get(val);
            if (dup[i] === undefined)
               dup[i] = "UNKNOWN_OBJECT (" + val.toString() + ")";
         }
         else
            dup[i] = val;
      }
      return dup;
   }

   function generateRecordedEventHandlerWithCallback(callback) {
      return function(e) {
         if (recording) {
            
            eventsInScope.push(recordEvent(e));
            callback.call(this, e);
            eventsInScope.pop();
         }
      }
   }

   function recordedEventHandler(e) {
      if (recording)
         recordEvent(e);
   }

   function recordEvent(e) {
      var record = new EventRecord(e);
      var recordList = allRecords;
      
      if (eventsInScope.length > 0) {
         recordList = eventsInScope[eventsInScope.length - 1].nestedEvents;
         if (recordList == null) 
            recordList = eventsInScope[eventsInScope.length - 1].nestedEvents = [];
      }
      if (mergeTypesTruthMap[e.type] && (recordList.length > 0)) {
         var tail = recordList[recordList.length-1];
         
         if ((tail.event.type == record.event.type) && (tail.event.currentTarget == record.event.currentTarget)) {
            tail.sequentialOccurrences++;
            return;
         }
      }
      recordList.push(record);
      return record;
   }

   Object.defineProperties(Node.prototype, {
      addRecordedEventListener: {
         enumerable: true, writable: true, configurable: true,
         value: function addRecordedEventListener(type, handler, capture) {
            if (handler == null)
               this.addEventListener(type, recordedEventHandler, capture);
            else {
               var subvertedHandler = generateRecordedEventHandlerWithCallback(handler);
               handlerMap.set(handler, subvertedHandler);
               this.addEventListener(type, subvertedHandler, capture);
            }
         }
      },
      removeRecordedEventListener: {
         enumerable: true, writable: true, configurable: true,
         value: function addRecordedEventListener(type, handler, capture) {
            var alternateHandlerUsed = handlerMap.get(handler);
            this.removeEventListenter(type, alternateHandlerUsed ? alternateHandlerUsed : recordedEventHandler, capture);
         }
      }
   });

})(window);
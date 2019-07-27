


const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/ResourceStatsDB.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const db = new ResourceStatsDB();


const wifiComponent = "wifi:0";
const mobileComponent = "mobile:1";
const cpuComponent = "cpu:0";
const gpsComponent = "gps:0";


const networkComponents = [wifiComponent, mobileComponent];
const powerComponents = [cpuComponent, gpsComponent];
const offset = (new Date()).getTimezoneOffset() * 60 * 1000;


function clearStore(store, callback) {
  db._dbNewTxn(store, "readwrite", function(aTxn, aStore) {
    aStore.openCursor().onsuccess = function (event) {
      let cursor = event.target.result;
      if (cursor){
        cursor.delete();
        cursor.continue();
      }
    };
  }, callback);
}


add_test(function prepareDatabase() {
  clearStore('power_stats_store', function() {
    clearStore('network_stats_store', function() {
      clearStore('alarm_store', function() {
        run_next_test();
      });
    });
  });
});


function dumpStore(store, callback) {
  db._dbNewTxn(store, "readonly", function(aTxn, aStore) {
    aStore.mozGetAll().onsuccess = function onsuccess(event) {
      aTxn.result = event.target.result;
    };
  }, callback);
}


add_test(function test_sampleRate() {
  var sampleRate = db.sampleRate;
  do_check_true(sampleRate > 0);

  db.sampleRate = 0;
  sampleRate = db.sampleRate;
  do_check_true(sampleRate > 0);

  run_next_test();
});


add_test(function test_maxStorageAge() {
  var maxStorageAge = db.maxStorageAge;
  do_check_true(maxStorageAge > 0);

  db.maxStorageAge = 0;
  maxStorageAge = db.maxStorageAge;
  do_check_true(maxStorageAge > 0);

  run_next_test();
});


function normalizeTime(aTimeStamp) {
  var sampleRate = db.sampleRate;

  return Math.floor((aTimeStamp - offset) / sampleRate) * sampleRate;
}



function generateNetworkRecord(aAppId, aServiceType, aComponents) {
  var result = [];
  var componentStats = {};
  var receivedBytes;
  var sentBytes;
  var totalReceivedBytes = 0;
  var totalSentBytes = 0;

  aComponents.forEach(function(comp) {
    
    receivedBytes = Math.floor(Math.random() * 1000);
    sentBytes = Math.floor(Math.random() * 1000);
    totalReceivedBytes += receivedBytes;
    totalSentBytes += sentBytes;

    
    componentStats[comp] = {
      receivedBytes: receivedBytes,
      sentBytes: sentBytes
    };

    
    result.push({
      appId: aAppId,
      serviceType: aServiceType,
      component: comp,
      receivedBytes: receivedBytes,
      sentBytes: sentBytes
    });
  });

  
  result.push({
    appId: aAppId,
    serviceType: aServiceType,
    component: "",
    receivedBytes: totalReceivedBytes,
    sentBytes: totalSentBytes
  });

  
  var record = { appId: aAppId,
                 serviceType: aServiceType,
                 componentStats: componentStats };

  return { record: record, result: result };
}



function generatePowerRecord(aAppId, aServiceType, aComponents) {
  var result = [];
  var componentStats = {};
  var consumedPower;
  var totalConsumedPower = 0;

  aComponents.forEach(function(comp) {
    
    consumedPower = Math.floor(Math.random() * 1000);
    totalConsumedPower += consumedPower;

    
    componentStats[comp] = consumedPower;

    
    result.push({
      appId: aAppId,
      serviceType: aServiceType,
      component: comp,
      consumedPower: consumedPower
    });
  });

  
  result.push({
    appId: aAppId,
    serviceType: aServiceType,
    component: "",
    consumedPower: totalConsumedPower
  });

  
  var record = { appId: aAppId,
                 serviceType: aServiceType,
                 componentStats: componentStats };

  return { record: record, result: result };
}


function checkNetworkStatsStore(aExpectedResult, aDumpResult, aTimestamp) {
  
  do_check_eq(aExpectedResult.length, aDumpResult.length);

  
  var mapArray = aExpectedResult.map(function(e) {return e.receivedBytes;});

  
  var index;
  var target;

  aDumpResult.forEach(function(stats) {
    index = 0;
    
    while ((index = mapArray.indexOf(stats.receivedBytes, index)) > -1) {
      
      target = aExpectedResult[index];
      if (target.appId != stats.appId ||
          target.serviceType != stats.serviceType ||
          target.component != stats.component ||
          target.sentBytes != stats.sentBytes ||
          aTimestamp != stats.timestamp) {
        index += 1;
        continue;
      } else {
        
        aExpectedResult.splice(index, 1);
        mapArray.splice(index, 1);
        break;
      }
    }
    do_check_neq(index, -1);
  });
  run_next_test();
}


function checkPowerStatsStore(aExpectedResult, aDumpResult, aTimestamp) {
  
  do_check_eq(aExpectedResult.length, aDumpResult.length);

  
  var mapArray = aExpectedResult.map(function(e) {return e.consumedPower;});

  
  var index;
  var target;

  aDumpResult.forEach(function(stats) {
    index = 0;
    
    while ((index = mapArray.indexOf(stats.consumedPower, index)) > -1) {
      
      target = aExpectedResult[index];
      if (target.appId != stats.appId ||
          target.serviceType != stats.serviceType ||
          target.component != stats.component ||
          aTimestamp != stats.timestamp) {
        index += 1;
        continue;
      } else {
        
        aExpectedResult.splice(index, 1);
        mapArray.splice(index, 1);
        break;
      }
    }
    do_check_neq(index, -1);
  });
  run_next_test();
}


function prepareNetworkStatsStore(recordArray, timestamp, callback) {
  
  clearStore("network_stats_store", function() {
    
    db.saveNetworkStats(recordArray, timestamp, callback);
  });
}


function preparePowerStatsStore(recordArray, timestamp, callback) {
  
  clearStore("power_stats_store", function() {
    
    db.savePowerStats(recordArray, timestamp, callback);
  });
}


add_test(function test_saveNetworkStats() {
  var appId = 1;
  var serviceType = "";

  
  var { record: record, result: expectedResult } =
    generateNetworkRecord(appId, serviceType, networkComponents);
  var recordArray = [record];
  var timestamp = Date.now();

  
  prepareNetworkStatsStore(recordArray, timestamp, function(error, callback) {
    
    do_check_eq(error, null);
    
    dumpStore("network_stats_store", function(error, result) {
      do_check_eq(error, null);
      checkNetworkStatsStore(expectedResult, result, normalizeTime(timestamp));
    });
  });
});


add_test(function test_savePowerStats() {
  var appId = 1;
  var serviceType = "";

  
  var { record: record, result: expectedResult } =
    generatePowerRecord(appId, serviceType, powerComponents);
  var recordArray = [record];
  var timestamp = Date.now();

  
  preparePowerStatsStore(recordArray, timestamp, function(error, callback) {
    
    do_check_eq(error, null);
    
    dumpStore("power_stats_store", function(error, result) {
      do_check_eq(error, null);
      checkPowerStatsStore(expectedResult, result, normalizeTime(timestamp));
    });
  });
});


add_test(function test_getNetworkStats() {
  var appId = 0;
  var manifestURL = "";
  var serviceType = "";
  var component = "";

  
  var { record: record, result: result } =
    generateNetworkRecord(appId, serviceType, networkComponents);
  var recordArray = [record];
  var expectedStats = result[result.length - 1]; 
  var timestamp = Date.now();
  var end = normalizeTime(timestamp) + offset;
  var start = end;

  
  prepareNetworkStatsStore(recordArray, timestamp, function(error, callback) {
    
    db.getStats("network", manifestURL, serviceType, component, start, end,
      function(error, result) {
      do_check_eq(error, null);

      
      do_check_eq(result.type, "network");
      do_check_eq(result.manifestURL, manifestURL);
      do_check_eq(result.serviceType, serviceType);
      do_check_eq(result.component, component);
      do_check_eq(result.start, start);
      do_check_eq(result.end, end);
      do_check_eq(result.sampleRate, db.sampleRate);
      do_check_true(Array.isArray(result.statsData));
      do_check_eq(result.statsData.length, 1);
      var stats = result.statsData[0];
      do_check_eq(stats.receivedBytes, expectedStats.receivedBytes);
      do_check_eq(stats.sentBytes, expectedStats.sentBytes);

      run_next_test(); 
    });
  });
});


add_test(function test_getPowerStats() {
  var appId = 0;
  var manifestURL = "";
  var serviceType = "";
  var component = "";

  
  var { record: record, result: result } =
    generatePowerRecord(appId, serviceType, powerComponents);
  var recordArray = [record];
  var expectedStats = result[result.length - 1]; 
  var timestamp = Date.now();
  var end = normalizeTime(timestamp) + offset;
  var start = end;

  
  preparePowerStatsStore(recordArray, timestamp, function(error, callback) {
    
    db.getStats("power", manifestURL, serviceType, component, start, end,
      function(error, result) {
      do_check_eq(error, null);

      
      do_check_eq(result.type, "power");
      do_check_eq(result.manifestURL, manifestURL);
      do_check_eq(result.serviceType, serviceType);
      do_check_eq(result.component, component);
      do_check_eq(result.start, start);
      do_check_eq(result.end, end);
      do_check_eq(result.sampleRate, db.sampleRate);
      do_check_true(Array.isArray(result.statsData));
      do_check_eq(result.statsData.length, 1);
      var stats = result.statsData[0];
      do_check_eq(stats.consumedPower, expectedStats.consumedPower);

      run_next_test(); 
    });
  });
});


add_test(function test_clearNetworkStats() {
  var appId = 0;
  var manifestURL = "";
  var serviceType = "";
  var component = "";

  
  var { record: record, result: result } =
    generateNetworkRecord(appId, serviceType, networkComponents);
  var recordArray = [record];
  var timestamp = Date.now();
  var end = normalizeTime(timestamp) + offset;
  var start = end;

  
  prepareNetworkStatsStore(recordArray, timestamp, function(error, callback) {
    
    db.clearStats("network", appId, serviceType, component, start, end,
      function(error, result) {
      do_check_eq(error, null);

      
      db.getStats("network", manifestURL, serviceType, component, start, end,
        function(error, result) {
        do_check_eq(result.statsData.length, 0);
        run_next_test();
      });
    });
  });
});


add_test(function test_clearPowerStats() {
  var appId = 0;
  var manifestURL = "";
  var serviceType = "";
  var component = "";

  
  var { record: record, result: result } =
    generatePowerRecord(appId, serviceType, powerComponents);
  var recordArray = [record];
  var timestamp = Date.now();
  var end = normalizeTime(timestamp) + offset;
  var start = end;

  
  preparePowerStatsStore(recordArray, timestamp, function(error, callback) {
    
    db.clearStats("power", appId, serviceType, component, start, end,
      function(error, result) {
      do_check_eq(error, null);

      
      db.getStats("power", manifestURL, serviceType, component, start, end,
        function(error, result) {
        do_check_eq(result.statsData.length, 0);
        run_next_test();
      });
    });
  });
});


add_test(function test_clearAllNetworkStats() {
  db.clearAllStats("network", function(error, result) {
    do_check_eq(error, null);
    run_next_test();
  });
});


add_test(function test_clearAllPowerStats() {
  db.clearAllStats("power", function(error, result) {
    do_check_eq(error, null);
    run_next_test();
  });
});


add_test(function test_getNetworkComponents() {
  var appId = 0;
  var serviceType = "";

  
  var { record: record, result: expectedResult } =
    generateNetworkRecord(appId, serviceType, networkComponents);
  var recordArray = [record];
  var timestamp = Date.now();

  
  prepareNetworkStatsStore(recordArray, timestamp, function(error, callback) {
    
    db.getComponents("network", function(error, result) {
      do_check_eq(error, null);
      do_check_true(Array.isArray(result));
      do_check_eq(result.length, networkComponents.length);

      
      result.forEach(function(component) {
        do_check_true(networkComponents.indexOf(component) > -1);
      });

      run_next_test(); 
    });
  });
});


add_test(function test_getPowerComponents() {
  var appId = 0;
  var serviceType = "";

  
  var { record: record, result: expectedResult } =
    generatePowerRecord(appId, serviceType, powerComponents);
  var recordArray = [record];
  var timestamp = Date.now();

  
  preparePowerStatsStore(recordArray, timestamp, function(error, callback) {
    
    db.getComponents("power", function(error, result) {
      do_check_eq(error, null);
      do_check_true(Array.isArray(result));
      do_check_eq(result.length, powerComponents.length);

      
      result.forEach(function(component) {
        do_check_true(powerComponents.indexOf(component) > -1);
      });

      run_next_test(); 
    });
  });
});


function generateAlarmObject(aType, aManifestURL, aServiceType, aComponent) {
  let alarm = {
    type: aType,
    component: aComponent,
    serviceType: aServiceType,
    manifestURL: aManifestURL,
    threshold: Math.floor(Math.random() * 1000),
    startTime: Math.floor(Math.random() * 1000),
    data: null
  };

  return alarm;
}


add_test(function test_addNetowrkAlarm() {
  var manifestURL = "";
  var serviceType = "";

  
  var alarm =
    generateAlarmObject("network", manifestURL, serviceType, wifiComponent);

  
  clearStore("alarm_store", function() {
    
    db.addAlarm(alarm, function(error, result) {
      
      do_check_eq(error, null);
      do_check_true(result > -1);
      let alarmId = result;

      
      dumpStore("alarm_store", function(error, result) {
        do_check_eq(error, null);
        do_check_true(Array.isArray(result));
        do_check_true(result.length == 1);
        do_check_eq(result[0].type, alarm.type);
        do_check_eq(result[0].manifestURL, alarm.manifestURL);
        do_check_eq(result[0].serviceType, alarm.serviceType);
        do_check_eq(result[0].component, alarm.component);
        do_check_eq(result[0].threshold, alarm.threshold);
        do_check_eq(result[0].startTime, alarm.startTime);
        do_check_eq(result[0].data, alarm.data);
        do_check_eq(result[0].alarmId, alarmId);

        run_next_test(); 
      });
    });
  });
});


add_test(function test_addPowerAlarm() {
  var manifestURL = "";
  var serviceType = "";

  
  var alarm =
    generateAlarmObject("power", manifestURL, serviceType, cpuComponent);

  
  clearStore("alarm_store", function() {
    
    db.addAlarm(alarm, function(error, result) {
      
      do_check_eq(error, null);
      do_check_true(result > -1);
      let alarmId = result;

      
      dumpStore("alarm_store", function(error, result) {
        do_check_eq(error, null);
        do_check_true(Array.isArray(result));
        do_check_true(result.length == 1);
        do_check_eq(result[0].type, alarm.type);
        do_check_eq(result[0].manifestURL, alarm.manifestURL);
        do_check_eq(result[0].serviceType, alarm.serviceType);
        do_check_eq(result[0].component, alarm.component);
        do_check_eq(result[0].threshold, alarm.threshold);
        do_check_eq(result[0].startTime, alarm.startTime);
        do_check_eq(result[0].data, alarm.data);
        do_check_eq(result[0].alarmId, alarmId);

        run_next_test(); 
      });
    });
  });
});



function addAlarmsToStore(alarms, index, callback) {
  var alarm = alarms[index++];
  if (index < alarms.length) {
    db.addAlarm(alarm, function(error, result) {
      alarm.alarmId = result;
      addAlarmsToStore(alarms, index, callback);
    });
  } else {
    db.addAlarm(alarm, function(error, result) {
      alarm.alarmId = result;
      callback(error, result);
    });
  }
}


function prepareAlarmStore(alarms, callback) {
  
  clearStore("alarm_store", function() {
    
    addAlarmsToStore(alarms, 0, callback);
  });
}


function compareAlarms(aExpectedResult, aResult) {
  
  do_check_eq(aExpectedResult.length, aResult.length);

  
  var mapArray = aExpectedResult.map(function(e) {return e.threshold;});

  
  var index;
  var target;

  aResult.forEach(function(alarm) {
    index = 0;
    
    while ((index = mapArray.indexOf(alarm.threshold, index)) > -1) {
      
      target = aExpectedResult[index];
      if (target.alarmId != alarm.alarmId ||
          target.type != alarm.type ||
          target.manifestURL != alarm.manifestURL ||
          target.serviceType != alarm.serviceType ||
          target.component != alarm.component ||
          target.data != alarm.data) {
        index += 1;
        continue;
      } else {
        
        aExpectedResult.splice(index, 1);
        mapArray.splice(index, 1);
        break;
      }
    }
    do_check_neq(index, -1);
  });
  run_next_test();
}


add_test(function test_getNetworkAlarms() {
  var manifestURL = "";
  var serviceType = "";
  var alarms = [];

  
  alarms.push(generateAlarmObject("network", manifestURL, serviceType,
                                  wifiComponent));
  alarms.push(generateAlarmObject("network", manifestURL, serviceType,
                                  wifiComponent));

  
  alarms.push(generateAlarmObject("network", manifestURL, serviceType,
                                  mobileComponent));

  
  prepareAlarmStore(alarms, function(error, result) {
    
    let options = {
      manifestURL: manifestURL,
      serviceType: serviceType,
      component: wifiComponent
    };
    db.getAlarms("network", options, function(error, result) {
      
      do_check_eq(error, null);

      
      
      
      alarms.pop();
      compareAlarms(alarms, result);
    });
  });
});


add_test(function test_getPowerAlarms() {
  var manifestURL = "";
  var serviceType = "";
  var alarms = [];

  
  alarms.push(generateAlarmObject("power", manifestURL, serviceType,
                                  cpuComponent));
  alarms.push(generateAlarmObject("power", manifestURL, serviceType,
                                  cpuComponent));

  
  alarms.push(generateAlarmObject("power", manifestURL, serviceType,
                                  gpsComponent));

  
  prepareAlarmStore(alarms, function(error, result) {
    
    let options = {
      manifestURL: manifestURL,
      serviceType: serviceType,
      component: cpuComponent
    };
    db.getAlarms("power", options, function(error, result) {
      
      do_check_eq(error, null);

      
      
      
      alarms.pop();
      compareAlarms(alarms, result);
    });
  });
});


add_test(function test_getAllNetworkAlarms() {
  var manifestURL = "";
  var serviceType = "";
  var alarms = [];

  
  alarms.push(generateAlarmObject("network", manifestURL, serviceType,
                                  wifiComponent));
  alarms.push(generateAlarmObject("network", manifestURL, serviceType,
                                  mobileComponent));

  
  alarms.push(generateAlarmObject("power", manifestURL, serviceType,
                                  cpuComponent));

  
  prepareAlarmStore(alarms, function(error, result) {
    
    let options = null;
    db.getAlarms("network", options, function(error, result) {
      
      do_check_eq(error, null);

      
      
      
      alarms.pop();
      compareAlarms(alarms, result);
    });
  });
});


add_test(function test_getAllPowerAlarms() {
  var manifestURL = "";
  var serviceType = "";
  var alarms = [];

  
  alarms.push(generateAlarmObject("power", manifestURL, serviceType,
                                  cpuComponent));
  alarms.push(generateAlarmObject("power", manifestURL, serviceType,
                                  gpsComponent));

  
  alarms.push(generateAlarmObject("network", manifestURL, serviceType,
                                  wifiComponent));

  
  prepareAlarmStore(alarms, function(error, result) {
    
    let options = null;
    db.getAlarms("power", options, function(error, result) {
      
      do_check_eq(error, null);

      
      
      
      alarms.pop();
      compareAlarms(alarms, result);
    });
  });
});


add_test(function test_removeNetworkAlarm() {
  var manifestURL = "";
  var serviceType = "";
  var alarms = [];

  
  alarms.push(generateAlarmObject("network", manifestURL, serviceType,
                                  wifiComponent));

  
  prepareAlarmStore(alarms, function(error, result) {
    var alarmId = result;
    
    db.removeAlarm("network", alarmId, function(error, result) {
      
      do_check_eq(result, true);

      
      dumpStore("alarm_store", function(error, result) {
        do_check_eq(error, null);
        do_check_true(Array.isArray(result));
        do_check_true(result.length === 0);

        run_next_test(); 
      });
    });
  });
});


add_test(function test_removePowerAlarm() {
  var manifestURL = "";
  var serviceType = "";
  var alarms = [];

  
  alarms.push(generateAlarmObject("power", manifestURL, serviceType,
                                  cpuComponent));

  
  prepareAlarmStore(alarms, function(error, result) {
    var alarmId = result;
    
    db.removeAlarm("power", alarmId, function(error, result) {
      
      do_check_eq(result, true);

      
      dumpStore("alarm_store", function(error, result) {
        do_check_eq(error, null);
        do_check_true(Array.isArray(result));
        do_check_true(result.length === 0);

        run_next_test(); 
      });
    });
  });
});


add_test(function test_removeAllNetworkAlarms() {
  var manifestURL = "";
  var serviceType = "";
  var alarms = [];

  
  alarms.push(generateAlarmObject("network", manifestURL, serviceType,
                                  wifiComponent));
  alarms.push(generateAlarmObject("network", manifestURL, serviceType,
                                  mobileComponent));

  
  alarms.push(generateAlarmObject("power", manifestURL, serviceType,
                                  cpuComponent));

  
  prepareAlarmStore(alarms, function(error, result) {
    
    db.removeAllAlarms("network", function(error, result) {
      
      do_check_eq(error, null);

      
      
      
      var alarm = alarms.pop(); 
      dumpStore("alarm_store", function(error, result) {
        do_check_eq(error, null);
        do_check_true(Array.isArray(result));
        do_check_true(result.length == 1);
        do_check_eq(result[0].type, alarm.type);
        do_check_eq(result[0].manifestURL, alarm.manifestURL);
        do_check_eq(result[0].serviceType, alarm.serviceType);
        do_check_eq(result[0].component, alarm.component);
        do_check_eq(result[0].threshold, alarm.threshold);
        do_check_eq(result[0].startTime, alarm.startTime);
        do_check_eq(result[0].data, alarm.data);
        do_check_eq(result[0].alarmId, alarm.alarmId);

        run_next_test(); 
      });
    });
  });
});


add_test(function test_removeAllPowerAlarms() {
  var manifestURL = "";
  var serviceType = "";
  var alarms = [];

  
  alarms.push(generateAlarmObject("power", manifestURL, serviceType,
                                  cpuComponent));
  alarms.push(generateAlarmObject("power", manifestURL, serviceType,
                                  gpsComponent));

  
  alarms.push(generateAlarmObject("network", manifestURL, serviceType,
                                  wifiComponent));

  
  prepareAlarmStore(alarms, function(error, result) {
    
    db.removeAllAlarms("power", function(error, result) {
      
      do_check_eq(error, null);

      
      
      
      var alarm = alarms.pop(); 
      dumpStore("alarm_store", function(error, result) {
        do_check_eq(error, null);
        do_check_true(Array.isArray(result));
        do_check_true(result.length == 1);
        do_check_eq(result[0].type, alarm.type);
        do_check_eq(result[0].manifestURL, alarm.manifestURL);
        do_check_eq(result[0].serviceType, alarm.serviceType);
        do_check_eq(result[0].component, alarm.component);
        do_check_eq(result[0].threshold, alarm.threshold);
        do_check_eq(result[0].startTime, alarm.startTime);
        do_check_eq(result[0].data, alarm.data);
        do_check_eq(result[0].alarmId, alarm.alarmId);

        run_next_test(); 
      });
    });
  });
});

function run_test() {
  do_get_profile();
  run_next_test();
}














function isLocalRtp(statObject) {
  return (typeof statObject === 'object' &&
          statObject.isRemote === false);
}











function outputPcStats(stats, label) {
  var outputStr = '\n\n';
  function appendOutput(line) {
    outputStr += line.toString() + '\n';
  }

  var firstRtp = true;
  for (var prop in stats) {
    if (isLocalRtp(stats[prop])) {
      var rtp = stats[prop];
      if (firstRtp) {
        appendOutput(label.toUpperCase() + ' STATS ' +
                     '(' + new Date(rtp.timestamp).toISOString() + '):');
        firstRtp = false;
      }
      appendOutput('  ' + rtp.id + ':');
      if (rtp.type === 'inboundrtp') {
        appendOutput('    bytesReceived: ' + rtp.bytesReceived);
        appendOutput('    jitter: ' + rtp.jitter);
        appendOutput('    packetsLost: ' + rtp.packetsLost);
        appendOutput('    packetsReceived: ' + rtp.packetsReceived);
      } else {
        appendOutput('    bytesSent: ' + rtp.bytesSent);
        appendOutput('    packetsSent: ' + rtp.packetsSent);
      }
    }
  }
  outputStr += '\n\n';
  dump(outputStr);
}


var _lastStats = {};










function verifyPcStats(stats, label) {
  const INCREASING_INBOUND_STAT_NAMES = [
    'bytesReceived',
    'packetsReceived'
  ];

  const INCREASING_OUTBOUND_STAT_NAMES = [
    'bytesSent',
    'packetsSent'
  ];

  if (_lastStats[label] !== undefined) {
    function verifyIncrease(rtpName, statNames) {
      var timestamp = new Date(stats[rtpName].timestamp).toISOString();

      statNames.forEach(function (statName) {
        ok(stats[rtpName][statName] > _lastStats[label][rtpName][statName],
           timestamp + '.' + label + '.' + rtpName + '.' + statName,
           label + '.' + rtpName + '.' + statName + ' increased (value=' +
           stats[rtpName][statName] + ')');
      });
    }

    for (var prop in stats) {
      if (isLocalRtp(stats[prop])) {
        if (stats[prop].type === 'inboundrtp') {
          verifyIncrease(prop, INCREASING_INBOUND_STAT_NAMES);
        } else {
          verifyIncrease(prop, INCREASING_OUTBOUND_STAT_NAMES);
        }
      }
    }
  }
  
  _lastStats[label] = stats;
}












function processPcStats(pc, label, operations) {
  pc.getStats(null, function (stats) {
    operations.forEach(function (operation) {
      operation(stats, label);
    });
  });
}









function verifyConnectionStatus(test) {
  const OPERATIONS = [outputPcStats, verifyPcStats];

  if (test.pcLocal) {
    processPcStats(test.pcLocal, 'LOCAL', OPERATIONS);
  }

  if (test.pcRemote) {
    processPcStats(test.pcRemote, 'REMOTE', OPERATIONS);
  }
}

















function generateIntervalCommand(callback, interval, duration, name) {
  interval = interval || 1000;
  duration = duration || 1000 * 3600 * 3;
  name = name || 'INTERVAL_COMMAND';

  return [
    name,
    function (test) {
      var startTime = Date.now();
      var intervalId = setInterval(function () {
        if (callback) {
          callback(test);
        }

        var timeElapsed = Date.now() - startTime;
        if (timeElapsed >= duration) {
          clearInterval(intervalId);
          test.next();
        }
      }, interval);
    }
  ]
}


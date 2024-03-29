









var https = require('https');
var fs = require('fs');
var child = require('child_process');
var Browserify = require('browserify');
var Dnode = require('dnode');
var Express = require('express');
var WebSocketServer = require('ws').Server;
var WebSocketStream = require('websocket-stream');







BotManager = function () {
  this.webSocketServer_ = null;
  this.bots_ = [];
  this.pendingConnections_ = [];
  this.androidDeviceManager_ = new AndroidDeviceManager();
}

BotManager.BotTypes = {
  CHROME : 'chrome',
  ANDROID_CHROME : 'android-chrome',
};

BotManager.prototype = {
  createBot_: function (name, botType, callback) {
    switch(botType) {
      case BotManager.BotTypes.CHROME:
        return new BrowserBot(name, callback);
      case BotManager.BotTypes.ANDROID_CHROME:
        return new AndroidChromeBot(name, this.androidDeviceManager_,
            callback);
      default:
        console.log('Error: Type ' + botType + ' not supported by rtc-Bot!');
        process.exit(1);
    }
  },

  spawnNewBot: function (name, botType, callback) {
    this.startWebSocketServer_();
    var bot = this.createBot_(name, botType, callback);
    this.bots_.push(bot);
    this.pendingConnections_.push(bot.onBotConnected.bind(bot));
  },

  startWebSocketServer_: function () {
    if (this.webSocketServer_) return;

    this.app_ = new Express();

    this.app_.use('/bot/api.js',
        this.serveBrowserifyFile_.bind(this,
          __dirname + '/bot/api.js'));

    this.app_.use('/bot/', Express.static(__dirname + '/bot'));

    var options = options = {
      key: fs.readFileSync('configurations/priv.pem', 'utf8'),
      cert: fs.readFileSync('configurations/cert.crt', 'utf8')
    };
    this.server_ = https.createServer(options, this.app_);

    this.webSocketServer_ = new WebSocketServer({ server: this.server_ });
    this.webSocketServer_.on('connection', this.onConnection_.bind(this));

    this.server_.listen(8080);
  },

  onConnection_: function (ws) {
    var callback = this.pendingConnections_.shift();
    callback(new WebSocketStream(ws));
  },

  serveBrowserifyFile_: function (file, request, result) {
    
    var browserify = new Browserify();
    browserify.add(file);
    browserify.bundle().pipe(result);
  }
}





Bot = function (name, callback) {
  this.name_ = name;
  this.onbotready_ = callback;
}

Bot.prototype = {
  log: function (msg) {
    console.log("bot:" + this.name_ + " > " + msg);
  },

  name: function () { return this.name_; },

  onBotConnected: function (stream) {
    this.log('Connected');
    this.stream_ = stream;
    this.dnode_ = new Dnode();
    this.dnode_.on('remote', this.onRemoteFromDnode_.bind(this));
    this.dnode_.pipe(this.stream_).pipe(this.dnode_);
  },

  onRemoteFromDnode_: function (remote) {
    this.onbotready_(remote);
  }
}





BrowserBot = function (name, callback) {
  Bot.call(this, name, callback);
  this.spawnBotProcess_();
}

BrowserBot.prototype = {
  spawnBotProcess_: function () {
    this.log('Spawning browser');
    child.exec('google-chrome "https://localhost:8080/bot/browser/"');
  },

  __proto__: Bot.prototype
}



AndroidChromeBot = function (name, androidDeviceManager, callback) {
  Bot.call(this, name, callback);
  androidDeviceManager.getNewDevice(function (serialNumber) {
    this.serialNumber_ = serialNumber;
    this.spawnBotProcess_();
  }.bind(this));
}

AndroidChromeBot.prototype = {
  spawnBotProcess_: function () {
    this.log('Spawning Android device with serial ' + this.serialNumber_);
    var runChrome = 'adb -s ' + this.serialNumber_ + ' shell am start ' +
    '-n com.android.chrome/com.google.android.apps.chrome.Main ' +
    '-d https://localhost:8080/bot/browser/';
    child.exec(runChrome, function (error, stdout, stderr) {
      if (error) {
        this.log(error);
        process.exit(1);
      }
      this.log('Opening Chrome for Android...');
      this.log(stdout);
    }.bind(this));
  },

  __proto__: Bot.prototype
}

AndroidDeviceManager = function () {
  this.connectedDevices_ = [];
}

AndroidDeviceManager.prototype = {
  getNewDevice: function (callback) {
    this.listDevices_(function (devices) {
      for (var i = 0; i < devices.length; i++) {
        if (!this.connectedDevices_[devices[i]]) {
          this.connectedDevices_[devices[i]] = devices[i];
          callback(this.connectedDevices_[devices[i]]);
          return;
        }
      }
      if (devices.length == 0) {
        console.log('Error: No connected devices!');
      } else {
        console.log('Error: There is no enough connected devices.');
      }
      process.exit(1);
    }.bind(this));
  },

  listDevices_: function (callback) {
    child.exec('adb devices' , function (error, stdout, stderr) {
      var devices = [];
      if (error || stderr) {
        console.log(error || stderr);
      }
      if (stdout) {
        
        
        
        var tempList = stdout.split("\n").slice(1);
        for (var i = 0; i < tempList.length; i++) {
          if (tempList[i] == "") {
            continue;
          }
          devices.push(tempList[i].split("\t")[0]);
        }
      }
      callback(devices);
    });
  },
}
module.exports = BotManager;

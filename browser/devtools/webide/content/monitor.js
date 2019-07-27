



const Cu = Components.utils;
Cu.import('resource:///modules/devtools/gDevTools.jsm');
const {require} = Cu.import('resource://gre/modules/devtools/Loader.jsm', {}).devtools;
const {Services} = Cu.import('resource://gre/modules/Services.jsm');
const {Devices} = Cu.import("resource://gre/modules/devtools/Devices.jsm");
const {AppManager} = require('devtools/webide/app-manager');
const {AppActorFront} = require('devtools/app-actor-front');
const {Connection} = require('devtools/client/connection-manager');
const EventEmitter = require('devtools/toolkit/event-emitter');

window.addEventListener('load', function onLoad() {
  window.removeEventListener('load', onLoad);
  window.addEventListener('resize', Monitor.resize);
  window.addEventListener('unload', Monitor.unload);

  document.querySelector('#close').onclick = () => {
    window.parent.UI.openProject();
  };

  Monitor.load();
});
















let Monitor = {

  apps: new Map(),
  graphs: new Map(),
  front: null,
  socket: null,
  wstimeout: null,
  b2ginfo: false,
  b2gtimeout: null,

  


  update: function(data, fallback) {
    if (Array.isArray(data)) {
      data.forEach(d => Monitor.update(d, fallback));
      return;
    }

    if (Monitor.b2ginfo && data.graph === 'USS') {
      
      
      return;
    }

    if (fallback) {
      for (let key in fallback) {
        if (!data[key]) {
          data[key] = fallback[key];
        }
      }
    }

    let graph = Monitor.graphs.get(data.graph);
    if (!graph) {
      let element = document.createElement('div');
      element.classList.add('graph');
      document.body.appendChild(element);

      graph = new Graph(data.graph, element);
      Monitor.resize(); 
      Monitor.graphs.set(data.graph, graph);
    }
    graph.update(data);
  },

  


  load: function() {
    AppManager.on('app-manager-update', Monitor.onAppManagerUpdate);
    Monitor.connectToRuntime();
    Monitor.connectToWebSocket();
  },

  


  unload: function() {
    AppManager.off('app-manager-update', Monitor.onAppManagerUpdate);
    Monitor.disconnectFromRuntime();
    Monitor.disconnectFromWebSocket();
  },

  


  resize: function() {
    for (let graph of Monitor.graphs.values()) {
      graph.resize();
    }
  },

  


  onAppManagerUpdate: function(event, what, details) {
    switch (what) {
      case 'list-tabs-response':
        Monitor.connectToRuntime();
        break;
      case 'connection':
        if (AppManager.connection.status == Connection.Status.DISCONNECTED) {
          Monitor.disconnectFromRuntime();
        }
        break;
    }
  },

  


  connectToRuntime: function() {
    Monitor.pollB2GInfo();
    let client = AppManager.connection && AppManager.connection.client;
    let resp = AppManager._listTabsResponse;
    if (client && resp && !Monitor.front) {
      Monitor.front = new AppActorFront(client, resp);
      Monitor.front.watchApps(Monitor.onRuntimeAppEvent);
    }
  },

  


  disconnectFromRuntime: function() {
    Monitor.unpollB2GInfo();
    if (Monitor.front) {
      Monitor.front.unwatchApps(Monitor.onRuntimeAppEvent);
      Monitor.front = null;
    }
  },

  


  connectToWebSocket: function() {
    let webSocketURL = Services.prefs.getCharPref('devtools.webide.monitorWebSocketURL');
    try {
      Monitor.socket = new WebSocket(webSocketURL);
      Monitor.socket.onmessage = function(event) {
        Monitor.update(JSON.parse(event.data));
      };
      Monitor.socket.onclose = function() {
        Monitor.wstimeout = setTimeout(Monitor.connectToWebsocket, 1000);
      };
    } catch(e) {
      Monitor.wstimeout = setTimeout(Monitor.connectToWebsocket, 1000);
    }
  },

  


  disconnectFromWebSocket: function() {
    clearTimeout(Monitor.wstimeout);
    if (Monitor.socket) {
      Monitor.socket.onclose = () => {};
      Monitor.socket.close();
    }
  },

  


  onRuntimeAppEvent: function(type, app) {
    if (type !== 'appOpen' && type !== 'appClose') {
      return;
    }

    let client = AppManager.connection.client;
    app.getForm().then(form => {
      if (type === 'appOpen') {
        app.monitorClient = new MonitorClient(client, form);
        app.monitorClient.start();
        app.monitorClient.on('update', Monitor.onRuntimeUpdate);
        Monitor.apps.set(form.monitorActor, app);
      } else {
        let app = Monitor.apps.get(form.monitorActor)
        if (app) {
          app.monitorClient.stop(() => app.monitorClient.destroy());
          Monitor.apps.delete(form.monitorActor);
        }
      }
    });
  },

  


  onRuntimeUpdate: function(type, packet) {
    let fallback = {}, app = Monitor.apps.get(packet.from);
    if (app) {
      fallback.curve = app.manifest.name
    }
    Monitor.update(packet.data, fallback);
  },

  







  pollB2GInfo: function() {
    if (AppManager.selectedRuntime) {
      let id = AppManager.selectedRuntime.id;
      let device = Devices.getByName(id);
      if (device && device.shell) {
        device.shell('b2g-info').then(s => {
          let lines = s.split('\n');
          let line = '';

          
          
          while (line.indexOf('NAME') < 0) {
            if (lines.length < 1) {
              
              Monitor.unpollB2GInfo();
              return;
            }
            line = lines.shift();
          }
          let namelength = line.indexOf('NAME') + 'NAME'.length;
          let ussindex = line.slice(namelength).split(/\s+/).indexOf('USS');

          
          
          while (lines.length > 0 && lines[0].length > namelength) {
            line = lines.shift();
            let name = line.slice(0, namelength);
            let uss = line.slice(namelength).split(/\s+/)[ussindex];
            Monitor.update({
              curve: name.trim(),
              value: 1024 * 1024 * parseFloat(uss) 
            }, {
              
              
              graph: 'USS'
            });
          }
        });
      }
    }
    Monitor.b2ginfo = true;
    Monitor.b2gtimeout = setTimeout(Monitor.pollB2GInfo, 350);
  },

  


  unpollB2GInfo: function() {
    clearTimeout(Monitor.b2gtimeout);
    Monitor.b2ginfo = false;
  }

};






function MonitorClient(client, form) {
  this.client = client;
  this.actor = form.monitorActor;
  this.events = ['update'];

  EventEmitter.decorate(this);
  this.client.registerClient(this);
}
MonitorClient.prototype.destroy = function () {
  this.client.unregisterClient(this);
}
MonitorClient.prototype.detach = function () {}
MonitorClient.prototype.start = function () {
  this.client.request({
    to: this.actor,
    type: 'start'
  });
}
MonitorClient.prototype.stop = function (callback) {
  this.client.request({
    to: this.actor,
    type: 'stop'
  }, callback);
}





function Graph(name, element) {
  this.name = name;
  this.element = element;
  this.curves = new Map();
  this.events = new Map();
  this.ignored = new Set();
  this.enabled = true;
  this.request = null;

  this.x = d3.time.scale();
  this.y = d3.scale.linear();

  this.xaxis = d3.svg.axis().scale(this.x).orient('bottom');
  this.yaxis = d3.svg.axis().scale(this.y).orient('left');

  this.xformat = d3.time.format('%I:%M:%S');
  this.yformat = this.formatter(1);
  this.yaxis.tickFormat(this.formatter(0));

  this.line = d3.svg.line().interpolate('linear')
    .x(function(d) { return this.x(d.time); })
    .y(function(d) { return this.y(d.value); });

  this.color = d3.scale.category10();

  this.svg = d3.select(element).append('svg').append('g')
    .attr('transform', 'translate(' + this.margin.left + ',' + this.margin.top + ')');

  this.xelement = this.svg.append('g').attr('class', 'x axis').call(this.xaxis);
  this.yelement = this.svg.append('g').attr('class', 'y axis').call(this.yaxis);

  
  let xruler = this.xruler = this.svg.select('.x.axis').append('g').attr('class', 'x ruler');
  xruler.append('line').attr('y2', 6);
  xruler.append('line').attr('stroke-dasharray', '1,1');
  xruler.append('text').attr('y', 9).attr('dy', '.71em');

  let yruler = this.yruler = this.svg.select('.y.axis').append('g').attr('class', 'y ruler');
  yruler.append('line').attr('x2', -6);
  yruler.append('line').attr('stroke-dasharray', '1,1');
  yruler.append('text').attr('x', -9).attr('dy', '.32em');

  let self = this;

  d3.select(element).select('svg')
    .on('mousemove', function() {
      let mouse = d3.mouse(this);
      self.mousex = mouse[0] - self.margin.left,
      self.mousey = mouse[1] - self.margin.top;

      xruler.attr('transform', 'translate(' + self.mousex + ',0)');
      yruler.attr('transform', 'translate(0,' + self.mousey + ')');
    });
    



  this.mousex = this.mousey = -500;

  let sidebar = d3.select(this.element).append('div').attr('class', 'sidebar');
  let title = sidebar.append('label').attr('class', 'graph-title');

  title.append('input')
    .attr('type', 'checkbox')
    .attr('checked', 'true')
    .on('click', function() { self.toggle(); });
  title.append('span').text(this.name);

  this.legend = sidebar.append('div').attr('class', 'legend');

  this.resize = this.resize.bind(this);
  this.render = this.render.bind(this);
  this.averages = this.averages.bind(this);

  setInterval(this.averages, 1000);

  this.resize();
}

Graph.prototype = {

  



  margin: {
    top: 10,
    right: 150,
    bottom: 20,
    left: 50
  },

  


  toggle: function() {
    if (this.enabled) {
      this.element.classList.add('disabled');
      this.enabled = false;
    } else {
      this.element.classList.remove('disabled');
      this.enabled = true;
    }
    Monitor.resize();
  },

  



  resize: function() {
    let style = getComputedStyle(this.element),
        height = parseFloat(style.height) - this.margin.top - this.margin.bottom,
        width = parseFloat(style.width) - this.margin.left - this.margin.right;

    d3.select(this.element).select('svg')
      .attr('width', width + this.margin.left)
      .attr('height', height + this.margin.top + this.margin.bottom);

    this.x.range([0, width]);
    this.y.range([height, 0]);

    this.xelement.attr('transform', 'translate(0,' + height + ')')
    this.xruler.select('line[stroke-dasharray]').attr('y2', -height);
    this.yruler.select('line[stroke-dasharray]').attr('x2', width);
  },

  




  rescale: function() {
    let gettime = v => { return v.time; },
        getvalue = v => { return v.value; },
        ignored = c => { return this.ignored.has(c.id); };

    let xmin = null, xmax = null, ymin = null, ymax = null;
    for (let curve of this.curves.values()) {
      if (ignored(curve)) {
        continue;
      }
      if (xmax == null || curve.xmax > xmax) {
        xmax = curve.xmax;
      }
      if (xmin == null || curve.xmin < xmin) {
        xmin = curve.xmin;
      }
      if (ymax == null || curve.ymax > ymax) {
        ymax = curve.ymax;
      }
      if (ymin == null || curve.ymin < ymin) {
        ymin = curve.ymin;
      }
    }
    for (let event of this.events.values()) {
      if (ignored(event)) {
        continue;
      }
      if (xmax == null || event.xmax > xmax) {
        xmax = event.xmax;
      }
      if (xmin == null || event.xmin < xmin) {
        xmin = event.xmin;
      }
    }

    let oldxdomain = this.x.domain();
    if (xmin != null && xmax != null) {
      this.x.domain([xmin, xmax]);
      let newxdomain = this.x.domain();
      if (newxdomain[0] !== oldxdomain[0] || newxdomain[1] !== oldxdomain[1]) {
        this.xelement.call(this.xaxis);
      }
    }

    let oldydomain = this.y.domain();
    if (ymin != null && ymax != null) {
      this.y.domain([ymin, ymax]).nice();
      let newydomain = this.y.domain();
      if (newydomain[0] !== oldydomain[0] || newydomain[1] !== oldydomain[1]) {
        this.yelement.call(this.yaxis);
      }
    }
  },

  


  update: function(data) {
    delete data.graph;

    let time = data.time || Date.now();
    delete data.time;

    let curve = data.curve;
    delete data.curve;

    
    if ('value' in data) {
      this.push(this.curves, curve, [{time: time, value: data.value}]);
      delete data.value;
    }

    
    if ('values' in data) {
      this.push(this.curves, curve, data.values);
      delete data.values;
    }

    
    
    if ('event' in data) {
      this.push(this.events, data.event, [{time: time, value: data.duration}]);
      delete data.event;
      delete data.duration;
    }

    
    for (let key in data) {
      this.push(this.curves, key, [{time: time, value: data[key]}]);
    }

    
    if (this.enabled && !this.request) {
      this.request = requestAnimationFrame(this.render);
    }
  },

  


  push: function(collection, id, values) {

    
    let item = collection.get(id);
    if (!item) {
      item = { id: id, values: [], xmin: null, xmax: null, ymin: 0, ymax: null, average: 0 };
      collection.set(id, item);
    }

    for (let v of values) {
      let time = new Date(v.time), value = +v.value;
      
      if (item.xmax == null || time > item.xmax) {
        item.xmax = time;
      }
      if (item.xmin == null || time < item.xmin) {
        item.xmin = time;
      }
      if (item.ymax == null || value > item.ymax) {
        item.ymax = value;
      }
      if (item.ymin == null || value < item.ymin) {
        item.ymin = value;
      }
      
      item.values.push({ time: time, value: value });
    }
  },

  


  render: function() {
    this.request = null;
    this.rescale();


    

    let self = this,
        getid = d => { return d.id; },
        gettime = d => { return d.time.getTime(); },
        getline = d => { return self.line(d.values); },
        getcolor = d => { return self.color(d.id); },
        getvalues = d => { return d.values; },
        ignored = d => { return self.ignored.has(d.id); };

    
    let curvedata = [...this.curves.values()],
        eventdata = [...this.events.values()],
        data = curvedata.concat(eventdata);


    

    
    let curves = this.svg.selectAll('.curve').data(curvedata, getid);

    
    curves.enter().append('g').attr('class', 'curve').append('path')
      .style('stroke', getcolor);

    
    curves.exit().remove();

    
    this.svg.selectAll('.curve').select('path')
      .attr('d', d => { return ignored(d) ? '' : getline(d); });

    let height = parseFloat(getComputedStyle(this.element).height) - this.margin.top - this.margin.bottom;


    

    
    let events = this.svg.selectAll('.event-slot').data(eventdata, getid);

    
    events.enter().append('g').attr('class', 'event-slot');

    
    events.exit().remove();

    
    let lines = this.svg.selectAll('.event-slot')
      .style('stroke', d => { return ignored(d) ? 'none' : getcolor(d); })
    .selectAll('.event')
      .data(getvalues, gettime);

    
    lines.enter().append('line').attr('class', 'event').attr('y2', height);

    
    lines.exit().remove();

    
    this.svg.selectAll('.event')
      .attr('transform', d => { return 'translate(' + self.x(d.time) + ',0)'; });


    

    
    

    
    this.xruler.select('text').text(self.xformat(self.x.invert(self.mousex)));
    this.yruler.select('text').text(self.yformat(self.y.invert(self.mousey)));


    

    
    let legends = this.legend.selectAll('label').data(data, getid);

    
    legends.attr('title', c => { return 'Average: ' + self.yformat(c.average); });

    
    let newlegend = legends.enter().append('label');
    newlegend.append('input').attr('type', 'checkbox').attr('checked', 'true').on('click', function(c) {
      if (ignored(c)) {
        this.parentElement.classList.remove('disabled');
        self.ignored.delete(c.id);
      } else {
        this.parentElement.classList.add('disabled');
        self.ignored.add(c.id);
      }
      self.update({}); 
    });
    newlegend.append('span').attr('class', 'legend-color').style('background-color', getcolor);
    newlegend.append('span').attr('class', 'legend-id').text(getid);

    
    legends.exit().remove();
  },

  


  formatter: function(decimals) {
    return value => {
      
      if (Math.abs(value) < 1) return value.toFixed(decimals);
      
      let prefix = d3.formatPrefix(value);
      return prefix.scale(value).toFixed(decimals) + prefix.symbol;
    };
  },

  


  averages: function() {
    for (let c of this.curves.values()) {
      let length = c.values.length;
      if (length > 0) {
        let total = 0;
        c.values.forEach(v => total += v.value);
        c.average = (total / length);
      }
    }
  },

  


  bisectTime: d3.bisector(d => d.time).left,

  


  valuesAt: function(time) {
    let values = { time: time };

    for (let id of this.curves.keys()) {
      let curve = this.curves.get(id);

      
      let i = this.bisectTime(curve.values, time);
      if (i < 0) {
        
        values[id] = curve.values[0].value;
      } else if (i > curve.values.length-2) {
        
        values[id] = curve.values[curve.values.length-1].value;
      } else {
        
        let v1 = curve.values[i],
            v2 = curve.values[i+1],
            delta = (time - v1.time) / (v2.time - v1.time);
        values[id] = v1.value + (v2.value - v1.time) * delta;
      }
    }
    return values;
  }

};





const Cu = Components.utils;

const { require } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
const { GetDevices, GetDeviceString } = require("devtools/shared/devices");
const { Services } = Cu.import("resource://gre/modules/Services.jsm");
const { Simulators, Simulator } = require("devtools/webide/simulators");
const EventEmitter = require('devtools/toolkit/event-emitter');
const promise = require("promise");
const utils = require("devtools/webide/utils");

const Strings = Services.strings.createBundle("chrome://browser/locale/devtools/webide.properties");

let SimulatorEditor = {

  
  _addons: {},

  
  _devices: {},

  
  _deviceOptions: [],

  
  _form: null,

  
  _simulator: null,

  
  init() {
    let promises = [];

    
    let form = this._form;
    if (!form) {
      
      form = this._form = document.querySelector("#simulator-editor");
      form.addEventListener("change", this.update.bind(this));
      Simulators.on("configure", (e, simulator) => { this.edit(simulator) });
      
      let deviceFields = form.querySelectorAll("*[data-device]");
      this._deviceOptions = [].map.call(deviceFields, field => field.name);
    }

    
    function opt(select, value, text) {
      let option = document.createElement("option");
      option.value = value;
      option.textContent = text;
      select.appendChild(option);
    }

    
    promises.push(Simulators.findSimulatorAddons().then(addons => {
      this._addons = {};
      form.version.innerHTML = "";
      form.version.classList.remove("custom");
      addons.forEach(addon => {
        this._addons[addon.id] = addon;
        opt(form.version, addon.id, addon.name);
      });
      opt(form.version, "custom", "");
      opt(form.version, "pick", Strings.GetStringFromName("simulator_custom_binary"));
    }));

    
    form.profile.innerHTML = "";
    form.profile.classList.remove("custom");
    opt(form.profile, "default", Strings.GetStringFromName("simulator_default_profile"));
    opt(form.profile, "custom", "");
    opt(form.profile, "pick", Strings.GetStringFromName("simulator_custom_profile"));

    
    form.device.innerHTML = "";
    form.device.classList.remove("custom");
    opt(form.device, "custom", Strings.GetStringFromName("simulator_custom_device"));
    promises.push(GetDevices().then(devices => {
      devices.TYPES.forEach(type => {
        let b2gDevices = devices[type].filter(d => d.firefoxOS);
        if (b2gDevices.length < 1) {
          return;
        }
        let optgroup = document.createElement("optgroup");
        optgroup.label = GetDeviceString(type);
        b2gDevices.forEach(device => {
          this._devices[device.name] = device;
          opt(optgroup, device.name, device.name);
        });
        form.device.appendChild(optgroup);
      });
    }));

    return promise.all(promises);
  },

  
  edit(simulator) {
    
    if (!simulator) {
      simulator = new Simulator(); 
      Simulators.add(simulator);
    }

    this._simulator = null;

    return this.init().then(() => {
      this._simulator = simulator;

      
      this._form.name.value = simulator.name;
      this.updateVersionSelector();
      this.updateProfileSelector();
      this.updateDeviceSelector();
      this.updateDeviceFields();
    });
  },

  
  close() {
    this._simulator = null;
    window.parent.UI.openProject();
  },

  
  restoreDefaults() {
    let simulator = this._simulator;
    this.version = simulator.addon.id;
    this.profile = "default";
    simulator.restoreDefaults();
    Simulators.emitUpdated();
    return this.edit(simulator);
  },

  
  deleteSimulator() {
    Simulators.remove(this._simulator);
    this.close();
  },

  
  updateSelector(selector, value) {
    selector.value = value;
    if (selector[selector.selectedIndex].value !== value) {
      selector.value = "custom";
      selector.classList.add("custom");
      selector[selector.selectedIndex].textContent = value;
    }
  },

  

  get version() {
    return this._simulator.options.b2gBinary || this._simulator.addon.id;
  },

  set version(value) {
    let form = this._form;
    let simulator = this._simulator;
    let oldVer = simulator.version;
    if (this._addons[value]) {
      
      simulator.addon = this._addons[value];
      simulator.options.b2gBinary = null;
    } else {
      
      simulator.options.b2gBinary = value;
      
    }
    
    if (form.name.value.contains(oldVer) && simulator.version !== oldVer) {
      let regex = new RegExp("(.*)" + oldVer);
      let name = form.name.value.replace(regex, "$1" + simulator.version);
      simulator.options.name = form.name.value = Simulators.uniqueName(name);
    }
  },

  updateVersionSelector() {
    this.updateSelector(this._form.version, this.version);
  },

  

  get profile() {
    return this._simulator.options.gaiaProfile || "default";
  },

  set profile(value) {
    this._simulator.options.gaiaProfile = (value == "default" ? null : value);
  },

  updateProfileSelector() {
    this.updateSelector(this._form.profile, this.profile);
  },

  

  get device() {
    let devices = this._devices;
    let simulator = this._simulator;

    
    for (let name in devices) {
      let match = true;
      for (let option of this._deviceOptions) {
        if (simulator.options[option] === devices[name][option]) {
          continue;
        }
        match = false;
        break;
      }
      if (match) {
        return name;
      }
    }
    return "custom";
  },

  set device(name) {
    let device = this._devices[name];
    if (!device) {
      return;
    }
    let form = this._form;
    let simulator = this._simulator;
    this._deviceOptions.forEach(option => {
      simulator.options[option] = form[option].value = device[option] || null;
    });
    
    
  },

  updateDeviceSelector() {
    this.updateSelector(this._form.device, this.device);
  },

  
  updateDeviceFields() {
    let form = this._form;
    let simulator = this._simulator;
    this._deviceOptions.forEach(option => {
      form[option].value = simulator.options[option];
    });
  },

  
  update(event) {
    let simulator = this._simulator;
    if (!simulator) {
      return;
    }
    let form = this._form;
    let input = event.target;
    switch (input.name) {
      case "name":
        simulator.options.name = input.value;
        break;
      case "version":
        switch (input.value) {
          case "pick":
            let file = utils.getCustomBinary(window);
            if (file) {
              this.version = file.path;
            }
            
            this.updateVersionSelector();
            break;
          case "custom":
            this.version = input[input.selectedIndex].textContent;
            break;
          default:
            this.version = input.value;
        }
        break;
      case "profile":
        switch (input.value) {
          case "pick":
            let directory = utils.getCustomProfile(window);
            if (directory) {
              this.profile = directory.path;
            }
            
            this.updateProfileSelector();
            break;
          case "custom":
            this.profile = input[input.selectedIndex].textContent;
            break;
          default:
            this.profile = input.value;
        }
        break;
      case "device":
        this.device = input.value;
        break;
      default:
        simulator.options[input.name] = input.value || null;
        this.updateDeviceSelector();
    }
    Simulators.emitUpdated();
  },
};

window.addEventListener("load", function onLoad() {
  document.querySelector("#close").onclick = e => {
    SimulatorEditor.close();
  };
  document.querySelector("#reset").onclick = e => {
    SimulatorEditor.restoreDefaults();
  };
  document.querySelector("#remove").onclick = e => {
    SimulatorEditor.deleteSimulator();
  };

  
  SimulatorEditor.edit(Simulators._lastConfiguredSimulator);
});

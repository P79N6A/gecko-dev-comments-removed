




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
let promise = Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js").Promise;
Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "devtools",
  "resource://gre/modules/devtools/Loader.jsm");

Object.defineProperty(this, "WebConsoleUtils", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/utils").Utils;
  },
  configurable: true,
  enumerable: true
});

XPCOMUtils.defineLazyGetter(this, "VARIABLES_SORTING_ENABLED", () =>
  Services.prefs.getBoolPref("devtools.debugger.ui.variables-sorting-enabled")
);

const MAX_LONG_STRING_LENGTH = 200000;

this.EXPORTED_SYMBOLS = ["VariablesViewController"];


















function VariablesViewController(aView, aOptions) {
  this.addExpander = this.addExpander.bind(this);

  this._getObjectClient = aOptions.getObjectClient;
  this._getLongStringClient = aOptions.getLongStringClient;
  this._releaseActor = aOptions.releaseActor;

  if (aOptions.overrideValueEvalMacro) {
    this._overrideValueEvalMacro = aOptions.overrideValueEvalMacro;
  }
  if (aOptions.getterOrSetterEvalMacro) {
    this._getterOrSetterEvalMacro = aOptions.getterOrSetterEvalMacro;
  }
  if (aOptions.simpleValueEvalMacro) {
    this._simpleValueEvalMacro = aOptions.simpleValueEvalMacro;
  }

  this._actors = new Set();
  this.view = aView;
  this.view.controller = this;
}

VariablesViewController.prototype = {
  


  _getterOrSetterEvalMacro: VariablesView.getterOrSetterEvalMacro,

  


  _overrideValueEvalMacro: VariablesView.overrideValueEvalMacro,

  


  _simpleValueEvalMacro: VariablesView.simpleValueEvalMacro,

  









  _populateFromLongString: function(aTarget, aGrip){
    let deferred = promise.defer();

    let from = aGrip.initial.length;
    let to = Math.min(aGrip.length, MAX_LONG_STRING_LENGTH);

    this._getLongStringClient(aGrip).substring(from, to, aResponse => {
      
      this.releaseActor(aGrip);

      
      aTarget.onexpand = null;
      aTarget.setGrip(aGrip.initial + aResponse.substring);
      aTarget.hideArrow();

      
      aTarget._retrieved = true;
      deferred.resolve();
    });

    return deferred.promise;
  },

  








  _populateFromObject: function(aTarget, aGrip) {
    let deferred = promise.defer();

    this._getObjectClient(aGrip).getPrototypeAndProperties(aResponse => {
      let { ownProperties, prototype } = aResponse;
      
      let safeGetterValues = aResponse.safeGetterValues || {};
      let sortable = VariablesView.isSortable(aGrip.class);

      
      
      for (let name of Object.keys(safeGetterValues)) {
        if (name in ownProperties) {
          ownProperties[name].getterValue = safeGetterValues[name].getterValue;
          ownProperties[name].getterPrototypeLevel = safeGetterValues[name]
                                                     .getterPrototypeLevel;
        } else {
          ownProperties[name] = safeGetterValues[name];
        }
      }

      
      if (ownProperties) {
        aTarget.addItems(ownProperties, {
          
          sorted: sortable,
          
          callback: this.addExpander
        });
      }

      
      if (prototype && prototype.type != "null") {
        let proto = aTarget.addItem("__proto__", { value: prototype });
        
        this.addExpander(proto, prototype);
      }

      
      aTarget._retrieved = true;
      this.view.commitHierarchy();
      deferred.resolve();
    });

    return deferred.promise;
  },

  








  addExpander: function(aTarget, aSource) {
    
    if (aTarget.getter || aTarget.setter) {
      aTarget.evaluationMacro = this._overrideValueEvalMacro;

      let getter = aTarget.get("get");
      if (getter) {
        getter.evaluationMacro = this._getterOrSetterEvalMacro;
      }

      let setter = aTarget.get("set");
      if (setter) {
        setter.evaluationMacro = this._getterOrSetterEvalMacro;
      }
    } else {
      aTarget.evaluationMacro = this._simpleValueEvalMacro;
    }

    
    if (VariablesView.isPrimitive({ value: aSource })) {
      return;
    }

    
    if (WebConsoleUtils.isActorGrip(aSource) && aSource.type == "longString") {
      aTarget.showArrow();
    }

    
    aTarget.onexpand = () => this.expand(aTarget, aSource);

    
    
    if (aTarget.shouldPrefetch) {
      aTarget.addEventListener("mouseover", aTarget.onexpand, false);
    }

    
    for (let grip of [aTarget.value, aTarget.getter, aTarget.setter]) {
      if (WebConsoleUtils.isActorGrip(grip)) {
        this._actors.add(grip.actor);
      }
    }
  },

  










  expand: function(aTarget, aSource) {
    
    if (aTarget._fetched) {
      return aTarget._fetched;
    }

    let deferred = promise.defer();
    aTarget._fetched = deferred.promise;

    if (!aSource) {
      throw new Error("No actor grip was given for the variable.");
    }

    
    if (VariablesView.isVariable(aTarget)) {
      this._populateFromObject(aTarget, aSource).then(() => {
        deferred.resolve();
        
        this.view.emit("fetched", "properties", aTarget);
      });
      return deferred.promise;
    }

    switch (aSource.type) {
      case "longString":
        this._populateFromLongString(aTarget, aSource).then(() => {
          deferred.resolve();
          
          this.view.emit("fetched", "longString", aTarget);
        });
        break;
      case "with":
      case "object":
        this._populateFromObject(aTarget, aSource.object).then(() => {
          deferred.resolve();
          
          this.view.emit("fetched", "variables", aTarget);
        });
        break;
      case "block":
      case "function":
        
        let args = aSource.bindings.arguments;
        if (args) {
          for (let arg of args) {
            let name = Object.getOwnPropertyNames(arg)[0];
            let ref = aTarget.addItem(name, arg[name]);
            let val = arg[name].value;
            this.addExpander(ref, val);
          }
        }

        aTarget.addItems(aSource.bindings.variables, {
          
          sorted: VARIABLES_SORTING_ENABLED,
          
          callback: this.addExpander
        });

        
        
        

        deferred.resolve();
        break;
      default:
        let error = "Unknown Debugger.Environment type: " + aSource.type;
        Cu.reportError(error);
        deferred.reject(error);
    }

    return deferred.promise;
  },

  





  releaseActor: function(aActor){
    if (this._releaseActor) {
      this._releaseActor(aActor);
    }
    this._actors.delete(aActor);
  },

  





  releaseActors: function(aFilter) {
    for (let actor of this._actors) {
      if (!aFilter || aFilter(actor)) {
        this.releaseActor(actor);
      }
    }
  },
};












VariablesViewController.attach = function(aView, aOptions) {
  if (aView.controller) {
    return aView.controller;
  }
  return new VariablesViewController(aView, aOptions);
};

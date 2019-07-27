



"use strict";

this.EXPORTED_SYMBOLS = ["LoginRecipesContent", "LoginRecipesParent"];

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;
const REQUIRED_KEYS = ["hosts"];
const OPTIONAL_KEYS = ["description", "passwordSelector", "pathRegex", "usernameSelector"];
const SUPPORTED_KEYS = REQUIRED_KEYS.concat(OPTIONAL_KEYS);

Cu.importGlobalProperties(["URL"]);

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoginHelper",
                                  "resource://gre/modules/LoginHelper.jsm");

XPCOMUtils.defineLazyGetter(this, "log", () => LoginHelper.createLogger("LoginRecipes"));









function LoginRecipesParent(aOptions = { defaults: true }) {
  if (Services.appinfo.processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT) {
    throw new Error("LoginRecipesParent should only be used from the main process");
  }
  this._defaults = aOptions.defaults;
  this.reset();
}

LoginRecipesParent.prototype = {
  




  initializationPromise: null,

  


  _defaults: null,

  



  _recipesByHost: null,

  




  load(aRecipes) {
    let recipeErrors = 0;
    for (let rawRecipe of aRecipes.siteRecipes) {
      try {
        rawRecipe.pathRegex = rawRecipe.pathRegex ? new RegExp(rawRecipe.pathRegex) : undefined;
        this.add(rawRecipe);
      } catch (ex) {
        recipeErrors++;
        log.error("Error loading recipe", rawRecipe, ex);
      }
    }

    if (recipeErrors) {
      return Promise.reject(`There were ${recipeErrors} recipe error(s)`);
    }

    return Promise.resolve();
  },

  


  reset() {
    log.debug("Resetting recipes with defaults:", this._defaults);
    this._recipesByHost = new Map();

    if (this._defaults) {
      
      this.initializationPromise = this.load(DEFAULT_RECIPES).then(resolve => {
        return this;
      });
    } else {
      this.initializationPromise = Promise.resolve(this);
    }
  },

  




  add(recipe) {
    log.debug("Adding recipe:", recipe);
    let recipeKeys = Object.keys(recipe);
    let unknownKeys = recipeKeys.filter(key => SUPPORTED_KEYS.indexOf(key) == -1);
    if (unknownKeys.length > 0) {
      throw new Error("The following recipe keys aren't supported: " + unknownKeys.join(", "));
    }

    let missingRequiredKeys = REQUIRED_KEYS.filter(key => recipeKeys.indexOf(key) == -1);
    if (missingRequiredKeys.length > 0) {
      throw new Error("The following required recipe keys are missing: " + missingRequiredKeys.join(", "));
    }

    if (!Array.isArray(recipe.hosts)) {
      throw new Error("'hosts' must be a array");
    }

    if (!recipe.hosts.length) {
      throw new Error("'hosts' must be a non-empty array");
    }

    if (recipe.pathRegex && recipe.pathRegex.constructor.name != "RegExp") {
      throw new Error("'pathRegex' must be a regular expression");
    }

    const OPTIONAL_STRING_PROPS = ["description", "passwordSelector", "usernameSelector"];
    for (let prop of OPTIONAL_STRING_PROPS) {
      if (recipe[prop] && typeof(recipe[prop]) != "string") {
        throw new Error(`'${prop}' must be a string`);
      }
    }

    
    for (let host of recipe.hosts) {
      if (!this._recipesByHost.has(host)) {
        this._recipesByHost.set(host, new Set());
      }
      this._recipesByHost.get(host).add(recipe);
    }
  },

  





  getRecipesForHost(aHost) {
    let hostRecipes = this._recipesByHost.get(aHost);
    if (!hostRecipes) {
      return new Set();
    }

    return hostRecipes;
  },
};


let LoginRecipesContent = {
  





  _filterRecipesForForm(aRecipes, aForm) {
    let formDocURL = aForm.ownerDocument.location;
    let host = formDocURL.host;
    let hostRecipes = aRecipes;
    let recipes = new Set();
    log.debug("_filterRecipesForForm", aRecipes);
    if (!hostRecipes) {
      return recipes;
    }

    for (let hostRecipe of hostRecipes) {
      if (hostRecipe.pathRegex && !hostRecipe.pathRegex.test(formDocURL.pathname)) {
        continue;
      }
      recipes.add(hostRecipe);
    }

    return recipes;
  },

  







  getFieldOverrides(aRecipes, aForm) {
    let recipes = this._filterRecipesForForm(aRecipes, aForm);
    log.debug("getFieldOverrides: filtered recipes:", recipes);
    if (!recipes.size) {
      return null;
    }

    let chosenRecipe = null;
    
    for (let recipe of recipes) {
      if (!recipe.usernameSelector && !recipe.passwordSelector) {
        continue;
      }

      chosenRecipe = recipe;
      break;
    }

    return chosenRecipe;
  },

  




  queryLoginField(aParent, aSelector) {
    if (!aSelector) {
      return null;
    }
    let field = aParent.ownerDocument.querySelector(aSelector);
    if (!field) {
      log.warn("Login field selector wasn't matched:", aSelector);
      return null;
    }
    if (!(field instanceof aParent.ownerDocument.defaultView.HTMLInputElement)) {
      log.warn("Login field isn't an <input> so ignoring it:", aSelector);
      return null;
    }
    return field;
  },
};

const DEFAULT_RECIPES = {
  "siteRecipes": [
    {
      "description": "okta uses a hidden password field to disable filling",
      "hosts": ["mozilla.okta.com"],
      "passwordSelector": "#pass-signin"
    },
    {
      "description": "anthem uses a hidden password and username field to disable filling",
      "hosts": ["www.anthem.com"],
      "passwordSelector": "#LoginContent_txtLoginPass"
    },
    {
      "description": "An ephemeral password-shim field is incorrectly selected as the username field.",
      "hosts": ["www.discover.com"],
      "usernameSelector": "#login-account"
    }
  ]
};








function test() {
  initNetMonitor(SIMPLE_URL).then(([aTab, aDebuggee, aMonitor]) => {
    info("Starting test... ");

    
    
    requestLongerTimeout(3);

    
    
    
    let getView = () => aMonitor.panelWin.NetMonitorView;
    let getController = () => aMonitor.panelWin.NetMonitorController;

    let prefsToCheck = {
      filters: {
        
        newValue: ["html", "css"],
        
        
        validateValue: ($) => getView().RequestsMenu._activeFilters,
        
        
        modifyFrontend: ($, aValue) => aValue.forEach(e => getView().RequestsMenu.filterOn(e))
      },
      networkDetailsWidth: {
        newValue: ~~(Math.random() * 200 + 100),
        validateValue: ($) => ~~$("#details-pane").getAttribute("width"),
        modifyFrontend: ($, aValue) => $("#details-pane").setAttribute("width", aValue)
      },
      networkDetailsHeight: {
        newValue: ~~(Math.random() * 300 + 100),
        validateValue: ($) => ~~$("#details-pane").getAttribute("height"),
        modifyFrontend: ($, aValue) => $("#details-pane").setAttribute("height", aValue)
      }
      
    };

    function storeFirstPrefValues() {
      info("Caching initial pref values.");

      for (let name in prefsToCheck) {
        let currentValue = aMonitor.panelWin.Prefs[name];
        prefsToCheck[name].firstValue = currentValue;
      }
    }

    function validateFirstPrefValues() {
      info("Validating current pref values to the UI elements.");

      for (let name in prefsToCheck) {
        let currentValue = aMonitor.panelWin.Prefs[name];
        let firstValue = prefsToCheck[name].firstValue;
        let validateValue = prefsToCheck[name].validateValue;

        is(currentValue.toSource(), firstValue.toSource(),
          "Pref " + name + " should be equal to first value: " + firstValue);
        is(currentValue.toSource(), validateValue(aMonitor.panelWin.$).toSource(),
          "Pref " + name + " should validate: " + currentValue);
      }
    }

    function modifyFrontend() {
      info("Modifying UI elements to the specified new values.");

      for (let name in prefsToCheck) {
        let currentValue = aMonitor.panelWin.Prefs[name];
        let firstValue = prefsToCheck[name].firstValue;
        let newValue = prefsToCheck[name].newValue;
        let validateValue = prefsToCheck[name].validateValue;
        let modifyFrontend = prefsToCheck[name].modifyFrontend;

        modifyFrontend(aMonitor.panelWin.$, newValue);
        info("Modified UI element affecting " + name + " to: " + newValue);

        is(currentValue.toSource(), firstValue.toSource(),
          "Pref " + name + " should still be equal to first value: " + firstValue);
        isnot(currentValue.toSource(), newValue.toSource(),
          "Pref " + name + " should't yet be equal to second value: " + newValue);
        is(newValue.toSource(), validateValue(aMonitor.panelWin.$).toSource(),
          "The UI element affecting " + name + " should validate: " + newValue);
      }
    }

    function validateNewPrefValues() {
      info("Invalidating old pref values to the modified UI elements.");

      for (let name in prefsToCheck) {
        let currentValue = aMonitor.panelWin.Prefs[name];
        let firstValue = prefsToCheck[name].firstValue;
        let newValue = prefsToCheck[name].newValue;
        let validateValue = prefsToCheck[name].validateValue;

        isnot(currentValue.toSource(), firstValue.toSource(),
          "Pref " + name + " should't be equal to first value: " + firstValue);
        is(currentValue.toSource(), newValue.toSource(),
          "Pref " + name + " should now be equal to second value: " + newValue);
        is(newValue.toSource(), validateValue(aMonitor.panelWin.$).toSource(),
          "The UI element affecting " + name + " should validate: " + newValue);
      }
    }

    function resetFrontend() {
      info("Resetting UI elements to the cached initial pref values.");

      for (let name in prefsToCheck) {
        let currentValue = aMonitor.panelWin.Prefs[name];
        let firstValue = prefsToCheck[name].firstValue;
        let newValue = prefsToCheck[name].newValue;
        let validateValue = prefsToCheck[name].validateValue;
        let modifyFrontend = prefsToCheck[name].modifyFrontend;

        modifyFrontend(aMonitor.panelWin.$, firstValue);
        info("Modified UI element affecting " + name + " to: " + firstValue);

        isnot(currentValue.toSource(), firstValue.toSource(),
          "Pref " + name + " should't yet be equal to first value: " + firstValue);
        is(currentValue.toSource(), newValue.toSource(),
          "Pref " + name + " should still be equal to second value: " + newValue);
        is(firstValue.toSource(), validateValue(aMonitor.panelWin.$).toSource(),
          "The UI element affecting " + name + " should validate: " + firstValue);
      }
    }

    function testBottom() {
      info("Testing prefs reload for a bottom host.");
      storeFirstPrefValues();

      
      validateFirstPrefValues();
      modifyFrontend();

      return restartNetMonitor(aMonitor)
        .then(([,, aNewMonitor]) => {
          aMonitor = aNewMonitor;

          
          validateNewPrefValues();
          resetFrontend();

          return restartNetMonitor(aMonitor);
        })
        .then(([,, aNewMonitor]) => {
          aMonitor = aNewMonitor;

          
          validateFirstPrefValues();
        });
    }

    function testSide() {
      info("Moving toolbox to the side...");

      return aMonitor._toolbox.switchHost(Toolbox.HostType.SIDE)
        .then(() => {
          info("Testing prefs reload for a side host.");
          storeFirstPrefValues();

          
          validateFirstPrefValues();
          modifyFrontend();

          return restartNetMonitor(aMonitor);
        })
        .then(([,, aNewMonitor]) => {
          aMonitor = aNewMonitor;

          
          validateNewPrefValues();
          resetFrontend();

          return restartNetMonitor(aMonitor);
        })
        .then(([,, aNewMonitor]) => {
          aMonitor = aNewMonitor;

          
          validateFirstPrefValues();
        });
    }

    function testWindow() {
      info("Moving toolbox into a window...");

      return aMonitor._toolbox.switchHost(Toolbox.HostType.WINDOW)
        .then(() => {
          info("Testing prefs reload for a window host.");
          storeFirstPrefValues();

          
          validateFirstPrefValues();
          modifyFrontend();

          return restartNetMonitor(aMonitor);
        })
        .then(([,, aNewMonitor]) => {
          aMonitor = aNewMonitor;

          
          validateNewPrefValues();
          resetFrontend();

          return restartNetMonitor(aMonitor);
        })
        .then(([,, aNewMonitor]) => {
          aMonitor = aNewMonitor;

          
          validateFirstPrefValues();
        });
    }

    function cleanupAndFinish() {
      info("Moving toolbox back to the bottom...");

      aMonitor._toolbox.switchHost(Toolbox.HostType.BOTTOM)
        .then(() => teardown(aMonitor))
        .then(finish);
    }

    testBottom()
      .then(testSide)
      .then(testWindow)
      .then(cleanupAndFinish);
  });
}

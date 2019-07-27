






"use strict";




add_task(function* test_initialize() {
  FormAutofillTest.requestAutocompleteResponse = { canceled: true };
});




add_task(function* test_integration_override() {
  let overrideCalled = false;

  let newIntegrationFn = base => ({
    createRequestAutocompleteUI: Task.async(function* () {
      overrideCalled = true;
      return yield base.createRequestAutocompleteUI.apply(this, arguments);
    }),
  });

  FormAutofill.registerIntegration(newIntegrationFn);
  try {
    let ui = yield FormAutofill.integration.createRequestAutocompleteUI({});
    let result = yield ui.show();
    Assert.ok(result.canceled);
  } finally {
    FormAutofill.unregisterIntegration(newIntegrationFn);
  }

  Assert.ok(overrideCalled);
});





add_task(function* test_integration_override_error() {
  let overrideCalled = false;

  let errorIntegrationFn = base => { throw "Expected error." };

  let newIntegrationFn = base => ({
    createRequestAutocompleteUI: Task.async(function* () {
      overrideCalled = true;
      return yield base.createRequestAutocompleteUI.apply(this, arguments);
    }),
  });

  FormAutofill.registerIntegration(errorIntegrationFn);
  FormAutofill.registerIntegration(newIntegrationFn);
  try {
    let ui = yield FormAutofill.integration.createRequestAutocompleteUI({});
    let result = yield ui.show();
    Assert.ok(result.canceled);
  } finally {
    FormAutofill.unregisterIntegration(errorIntegrationFn);
    FormAutofill.unregisterIntegration(newIntegrationFn);
  }

  Assert.ok(overrideCalled);
});

add_task(terminationTaskFn);




function SanityChecker()  {}

SanityChecker.prototype.checkScenario = function(scenario) {
  
  
  test(function() {

    
    
    var expectedFields = SPEC_JSON["test_expansion_schema"];
    expectedFields["referrer_policy"] = SPEC_JSON["referrer_policy_schema"];

    assert_own_property(scenario, "subresource_path",
                        "Scenario has the path to the subresource.");

    for (var field in expectedFields) {
      assert_own_property(scenario, field,
                          "The scenario contains field " + field)
      assert_in_array(scenario[field], expectedFields[field],
                      "Scenario's " + field + " is one of: " +
                      expectedFields[field].join(", ")) + "."
    }

    
    assert_equals(scenario["source_protocol"] + ":", location.protocol,
                  "Protocol of the test page should match the scenario.")

  }, "[ReferrerPolicyTestCase] The test scenario is valid.");
}

SanityChecker.prototype.checkSubresourceResult = function(test,
                                                          scenario,
                                                          subresourceUrl,
                                                          result) {
  test.step(function() {
    assert_equals(Object.keys(result).length, 3);
    assert_own_property(result, "location");
    assert_own_property(result, "referrer");
    assert_own_property(result, "headers");

    
    if (scenario.subresource == "script-tag")
      return;

    
    assert_equals(result.location, subresourceUrl,
                  "Subresource reported location.");
  }, "Running a valid test scenario.");
};

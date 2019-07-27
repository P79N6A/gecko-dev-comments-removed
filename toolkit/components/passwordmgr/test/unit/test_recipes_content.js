






"use strict";

Cu.importGlobalProperties(["URL"]);

add_task(function* test_getFieldOverrides() {
  let recipes = new Set([
    { 
      hosts: ["example.com:8080"],
      passwordSelector: "#password",
      pathRegex: /^\/$/,
      usernameSelector: ".username",
    },
    { 
      hosts: ["example.com:8080"],
    },
    { 
      description: "best match",
      hosts: ["a.invalid", "example.com:8080", "other.invalid"],
      passwordSelector: "#password",
      pathRegex: /^\/first\/second\/$/,
      usernameSelector: ".username",
    },
  ]);

  let form = MockDocument.createTestDocument("http://localhost:8080/first/second/", "<form>").
             forms[0];
  let override = LoginRecipesContent.getFieldOverrides(recipes, form);
  Assert.strictEqual(override.description, "best match",
                     "Check the best field override recipe was returned");
  Assert.strictEqual(override.usernameSelector, ".username", "Check usernameSelector");
  Assert.strictEqual(override.passwordSelector, "#password", "Check passwordSelector");
});

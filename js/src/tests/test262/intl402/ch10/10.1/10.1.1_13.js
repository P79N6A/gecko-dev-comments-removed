







$INCLUDE("testIntl.js");

testOption(Intl.Collator, "numeric", "boolean", undefined, undefined, {isOptional: true});
testOption(Intl.Collator, "caseFirst", "string", ["upper", "lower", "false"], undefined, {isOptional: true});


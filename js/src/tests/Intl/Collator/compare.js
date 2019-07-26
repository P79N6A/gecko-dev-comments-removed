






var input = [
    "Argentina",
    "Oerlikon",
    "Offenbach",
    "Sverige",
    "Vaticano",
    "Zimbabwe",
    "la France",
    "¡viva España!",
    "Österreich",
    "中国",
    "日本",
    "한국",
];

var collator, expected;

function assertEqualArray(actual, expected, collator) {
    var description = JSON.stringify(collator.resolvedOptions());
    assertEq(actual.length, expected.length, "array length, " + description);
    for (var i = 0; i < actual.length; i++) {
        assertEq(actual[i], expected[i], "element " + i + ", " + description);
    }
}



collator = new Intl.Collator("en-US");
expected = [
    "¡viva España!",
    "Argentina",
    "la France",
    "Oerlikon",
    "Offenbach",
    "Österreich",
    "Sverige",
    "Vaticano",
    "Zimbabwe",
    "한국",
    "中国",
    "日本",
];
assertEqualArray(input.sort(collator.compare), expected, collator);



collator = new Intl.Collator("sv-SE");
expected = [
    "¡viva España!",
    "Argentina",
    "la France",
    "Oerlikon",
    "Offenbach",
    "Sverige",
    "Vaticano",
    "Zimbabwe",
    "Österreich",
    "한국",
    "中国",
    "日本",
];
assertEqualArray(input.sort(collator.compare), expected, collator);


collator = new Intl.Collator("sv-SE", {ignorePunctuation: true});
expected = [
    "Argentina",
    "la France",
    "Oerlikon",
    "Offenbach",
    "Sverige",
    "Vaticano",
    "¡viva España!",
    "Zimbabwe",
    "Österreich",
    "한국",
    "中国",
    "日本",
];
assertEqualArray(input.sort(collator.compare), expected, collator);




collator = new Intl.Collator("de-DE");
expected = [
    "¡viva España!",
    "Argentina",
    "la France",
    "Oerlikon",
    "Offenbach",
    "Österreich",
    "Sverige",
    "Vaticano",
    "Zimbabwe",
    "한국",
    "中国",
    "日本",
];
assertEqualArray(input.sort(collator.compare), expected, collator);




collator = new Intl.Collator("de-DE-u-co-phonebk");
expected = [
    "¡viva España!",
    "Argentina",
    "la France",
    "Oerlikon",
    "Österreich",
    "Offenbach",
    "Sverige",
    "Vaticano",
    "Zimbabwe",
    "한국",
    "中国",
    "日本",
];
assertEqualArray(input.sort(collator.compare), expected, collator);

reportCompare(0, 0, 'ok');








var testData = [
    {minimumSignificantDigits: 1, maximumSignificantDigits: 1, expected: "0"},
    {minimumSignificantDigits: 1, maximumSignificantDigits: 2, expected: "0"},
    {minimumSignificantDigits: 1, maximumSignificantDigits: 3, expected: "0"},
    {minimumSignificantDigits: 1, maximumSignificantDigits: 4, expected: "0"},
    {minimumSignificantDigits: 1, maximumSignificantDigits: 5, expected: "0"},
    {minimumSignificantDigits: 2, maximumSignificantDigits: 2, expected: "0.0"},
    {minimumSignificantDigits: 2, maximumSignificantDigits: 3, expected: "0.0"},
    {minimumSignificantDigits: 2, maximumSignificantDigits: 4, expected: "0.0"},
    {minimumSignificantDigits: 2, maximumSignificantDigits: 5, expected: "0.0"},
    {minimumSignificantDigits: 3, maximumSignificantDigits: 3, expected: "0.00"},
    {minimumSignificantDigits: 3, maximumSignificantDigits: 4, expected: "0.00"},
    {minimumSignificantDigits: 3, maximumSignificantDigits: 5, expected: "0.00"},
];

for (var i = 0; i < testData.length; i++) {
    var min = testData[i].minimumSignificantDigits;
    var max = testData[i].maximumSignificantDigits;
    var options = {minimumSignificantDigits: min, maximumSignificantDigits: max};
    var format = new Intl.NumberFormat("en-US", options);
    var actual = format.format(0);
    var expected = testData[i].expected;
    if (actual !== expected) {
        throw new Error("Wrong formatted string for 0 with " +
                        "minimumSignificantDigits " + min +
                        ", maximumSignificantDigits " + max +
                        ": expected \"" + expected +
                        "\", actual \"" + actual + "\"");
    }
}

reportCompare(true, true);

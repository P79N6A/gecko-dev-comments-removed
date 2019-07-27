






try {
    `
    a
    b
    c
    `;
    throw Error("error");
} catch (e) {
    assertEq(e.lineNumber, 14);
}

try {
    function tagThatThrows(...args) { throw new Error(); }

    tagThatThrows`
        multi-line
        template
        string`;
} catch (e) {
    var stackLines = e.stack.split('\n');
    var firstLine = stackLines[0].split(':');
    var secondLine = stackLines[1].split(':');
    var firstLineSize = firstLine.length;
    var secondLineSize = secondLine.length;
    assertEq(firstLine[firstLineSize - 2], "20");
    assertEq(firstLine[firstLineSize - 1], "45");
    assertEq(secondLine[secondLineSize - 2], "22");
    assertEq(secondLine[secondLineSize - 1], "5");
}

try {
    ` multi-line
        template
        with
        ${substitutionThatThrows()}`

} catch (e) {
    assertEq(e.lineNumber, 42);
}



reportCompare(0, 0, "ok");

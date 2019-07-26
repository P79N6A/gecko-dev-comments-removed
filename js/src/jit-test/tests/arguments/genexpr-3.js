

{
    let arguments = [];
    assertEq((arguments for (p in {a: 1})).next(), arguments);
}

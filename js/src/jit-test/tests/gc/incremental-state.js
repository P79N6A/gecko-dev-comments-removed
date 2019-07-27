



if ("gcstate" in this) {
    assertEq(gcstate(), "none");

    
    gc();
    assertEq(gcstate(), "none");

    
    gcslice(1000000);
    assertEq(gcstate(), "none");

    



    gczeal(0);
    gcslice(1);
    assertEq(gcstate(), "mark");
    gcslice(1000000);
    assertEq(gcstate(), "mark");
    gcslice(1000000);
    assertEq(gcstate(), "none");

    
    gczeal(8);
    gcslice(1);
    assertEq(gcstate(), "mark");
    gcslice(1);
    assertEq(gcstate(), "none");

    
    gczeal(9);
    gcslice(1);
    assertEq(gcstate(), "mark");
    gcslice(1);
    assertEq(gcstate(), "none");

    
    gczeal(10);
    gcslice(1000000);
    assertEq(gcstate(), "sweep");
    gcslice(1000000);
    assertEq(gcstate(), "none");
}

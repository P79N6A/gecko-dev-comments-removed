





if (typeof gczeal != 'undefined' && typeof gc != 'undefined') {
    try
    {
        try {
            __defineGetter__("x", gc)
        } catch (e) {}
        gczeal(1)
        print(x)(Array(-8))
    }
    catch(ex)
    {
    }
}


if (typeof gczeal !== 'undefined')
    gczeal(0)

reportCompare("no assertion failure", "no assertion failure", "bug 563210");



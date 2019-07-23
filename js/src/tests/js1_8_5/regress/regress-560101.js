try {
    Object.defineProperty(<x/>, "p", {});  
} catch (exc) {}
reportCompare(0, 0, "ok");
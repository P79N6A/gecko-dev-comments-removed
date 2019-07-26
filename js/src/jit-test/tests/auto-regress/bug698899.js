


o = evalcx("lazy").__proto__
gc()
try {
    o.watch()
} catch (e) {}
o.constructor()

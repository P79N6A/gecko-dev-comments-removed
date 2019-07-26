
evalcx("\
    try {\
        throw\"\"\
    } catch (\
        x if (function(){\
            x\
        })()\
    ) {}\
", newGlobal(""))


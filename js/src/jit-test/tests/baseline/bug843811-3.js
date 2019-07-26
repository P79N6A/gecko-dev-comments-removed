
Function("\
    try {\
        throw\"\"\
    } catch (\
        x if (function(){\
            x\
        })()\
    ) {}\
")()


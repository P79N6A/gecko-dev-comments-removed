
eval("\
    try {\
        throw\"\"\
    } catch (\
        x if (function(){\
            x\
        })()\
    ) {}\
")

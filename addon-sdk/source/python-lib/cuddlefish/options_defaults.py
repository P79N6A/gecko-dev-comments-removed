



def parse_options_defaults(options, jetpack_id):
    
    pref_list = []

    for pref in options:
        if ('value' in pref):
            value = pref["value"]

            if isinstance(value, float):
                continue
            elif isinstance(value, bool):
                value = str(pref["value"]).lower()
            elif isinstance(value, str): 
                value = "\"" + unicode(pref["value"]) + "\""
            elif isinstance(value, unicode):
                value = "\"" + pref["value"] + "\""
            else:
                value = str(pref["value"])

            pref_list.append("pref(\"extensions." + jetpack_id + "." + pref["name"] + "\", " + value + ");")

    return "\n".join(pref_list) + "\n"

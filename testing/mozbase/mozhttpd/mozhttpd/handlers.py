



import json

def json_response(func):
    """ Translates results of 'func' into a JSON response. """
    def wrap(*a, **kw):
        (code, data) = func(*a, **kw)
        json_data = json.dumps(data)
        return (code, { 'Content-type': 'application/json',
                        'Content-Length': len(json_data) }, json_data)

    return wrap

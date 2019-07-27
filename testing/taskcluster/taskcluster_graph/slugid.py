import uuid
import base64

def slugid():
    '''
    Logic and rational of this construct here:
    https://github.com/jonasfj/slugid/blob/29be40074646b97e5ed02da257918467fac07c4a/slugid.js#L46
    '''
    encoded = base64.b64encode(str(uuid.uuid4().bytes))
    encoded = encoded.replace('+', '-') 
    encoded = encoded.replace('/', '_') 
    encoded = encoded.replace('=', '') 

    return encoded

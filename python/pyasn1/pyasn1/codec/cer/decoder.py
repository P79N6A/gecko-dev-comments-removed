
from pyasn1.type import univ
from pyasn1.codec.ber import decoder
from pyasn1.compat.octets import oct2int
from pyasn1 import error

class BooleanDecoder(decoder.AbstractSimpleDecoder):
    protoComponent = univ.Boolean(0)
    def valueDecoder(self, fullSubstrate, substrate, asn1Spec, tagSet, length,
                     state, decodeFun, substrateFun):
        head, tail = substrate[:length], substrate[length:]
        if not head:
            raise error.PyAsn1Error('Empty substrate')
        byte = oct2int(head[0])
        
        
        
        if byte == 0xff:
            value = 1
        elif byte == 0x00:
            value = 0
        else:
            raise error.PyAsn1Error('Boolean CER violation: %s' % byte)
        return self._createComponent(asn1Spec, tagSet, value), tail

tagMap = decoder.tagMap.copy()
tagMap.update({
    univ.Boolean.tagSet: BooleanDecoder()
    })

typeMap = decoder.typeMap

class Decoder(decoder.Decoder): pass

decode = Decoder(tagMap, decoder.typeMap)

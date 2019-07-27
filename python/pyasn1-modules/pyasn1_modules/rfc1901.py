





from pyasn1.type import univ, namedtype, namedval

class Message(univ.Sequence):
    componentType = namedtype.NamedTypes(
        namedtype.NamedType('version', univ.Integer(namedValues = namedval.NamedValues(('version-2c', 1)))),
        namedtype.NamedType('community', univ.OctetString()),
        namedtype.NamedType('data', univ.Any())
        )


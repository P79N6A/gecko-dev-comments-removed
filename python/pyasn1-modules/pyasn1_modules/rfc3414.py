





from pyasn1.type import univ, namedtype, namedval, tag, constraint

class UsmSecurityParameters(univ.Sequence):
    componentType = namedtype.NamedTypes(
        namedtype.NamedType('msgAuthoritativeEngineID', univ.OctetString()),
        namedtype.NamedType('msgAuthoritativeEngineBoots', univ.Integer().subtype(subtypeSpec=constraint.ValueRangeConstraint(0, 2147483647))),
        namedtype.NamedType('msgAuthoritativeEngineTime', univ.Integer().subtype(subtypeSpec=constraint.ValueRangeConstraint(0, 2147483647))),
        namedtype.NamedType('msgUserName', univ.OctetString().subtype(subtypeSpec=constraint.ValueSizeConstraint(0, 32))),
        namedtype.NamedType('msgAuthenticationParameters', univ.OctetString()),
        namedtype.NamedType('msgPrivacyParameters', univ.OctetString())
        )

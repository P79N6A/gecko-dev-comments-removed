







from pyasn1.type import tag, namedtype, namedval, univ, constraint
from pyasn1_modules.rfc2459 import *
from pyasn1_modules import rfc2251

class Attributes(univ.SetOf):
    componentType = rfc2251.Attribute()

class Version(univ.Integer): pass

class CertificationRequestInfo(univ.Sequence):
    componentType = namedtype.NamedTypes(
        namedtype.NamedType('version', Version()),
        namedtype.NamedType('subject', Name()),
        namedtype.NamedType('subjectPublicKeyInfo', SubjectPublicKeyInfo()),
        namedtype.NamedType('attributes', Attributes().subtype(implicitTag=tag.Tag(tag.tagClassContext, tag.tagFormatConstructed, 0)))
    )

class Signature(univ.BitString): pass
class SignatureAlgorithmIdentifier(AlgorithmIdentifier): pass

class CertificationRequest(univ.Sequence):
    componentType = namedtype.NamedTypes(
        namedtype.NamedType('certificationRequestInfo', CertificationRequestInfo()),
        namedtype.NamedType('signatureAlgorithm', SignatureAlgorithmIdentifier()),
        namedtype.NamedType('signature', Signature())
    )

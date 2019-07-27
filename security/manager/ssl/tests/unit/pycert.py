





"""
Reads a certificate specification from stdin or a file and outputs a
signed x509 certificate with the desired properties.

The input format is as follows:

issuer:<string to use as the issuer common name>
subject:<string to use as the subject common name>
[version:<{1,2,3,4}>]
[issuerKey:alternate]
[subjectKey:alternate]
[extension:<extension name:<extension-specific data>>]
[...]

Known extensions are:
basicConstraints:[cA],[pathLenConstraint]
keyUsage:[digitalSignature,nonRepudiation,keyEncipherment,
          dataEncipherment,keyAgreement,keyCertSign,cRLSign]
extKeyUsage:[serverAuth,clientAuth,codeSigning,emailProtection
             nsSGC, # Netscape Server Gated Crypto
             OCSPSigning,timeStamping]
subjectAlternativeName:[<dNSName>,...]

Where:
  [] indicates an optional field or component of a field
  <> indicates a required component of a field
  {} indicates choice among a set of values
  [a,b,c] indicates a list of potential values, of which more than one
          may be used

For instance, the version field is optional. However, if it is
specified, it must have exactly one value from the set {1,2,3,4}.

In the future it will be possible to specify other properties of the
generated certificate (for example, its validity period, signature
algorithm, etc.). For now, those fields have reasonable default values.
Currently one shared RSA key is used for all signatures and subject
public key information fields. Specifying "issuerKey:alternate" or
"subjectKey:alternate" causes a different RSA key be used for signing
or as the subject public key information field, respectively.
"""

from pyasn1.codec.der import decoder
from pyasn1.codec.der import encoder
from pyasn1.type import constraint, tag, univ, useful
from pyasn1_modules import rfc2459
import base64
import datetime
import hashlib
import sys

import pykey

class UnknownBaseError(Exception):
    """Base class for handling unexpected input in this module."""
    def __init__(self, value):
        self.value = value
        self.category = 'input'

    def __str__(self):
        return 'Unknown %s type "%s"' % (self.category, repr(self.value))


class UnknownAlgorithmTypeError(UnknownBaseError):
    """Helper exception type to handle unknown algorithm types."""

    def __init__(self, value):
        UnknownBaseError.__init__(self, value)
        self.category = 'algorithm'


class UnknownParameterTypeError(UnknownBaseError):
    """Helper exception type to handle unknown input parameters."""

    def __init__(self, value):
        UnknownBaseError.__init__(self, value)
        self.category = 'parameter'


class UnknownExtensionTypeError(UnknownBaseError):
    """Helper exception type to handle unknown input extensions."""

    def __init__(self, value):
        UnknownBaseError.__init__(self, value)
        self.category = 'extension'


class UnknownKeyPurposeTypeError(UnknownBaseError):
    """Helper exception type to handle unknown key purposes."""

    def __init__(self, value):
        UnknownBaseError.__init__(self, value)
        self.category = 'keyPurpose'


class UnknownKeyTargetError(UnknownBaseError):
    """Helper exception type to handle unknown key targets."""

    def __init__(self, value):
        UnknownBaseError.__init__(self, value)
        self.category = 'key target'


class UnknownVersionError(UnknownBaseError):
    """Helper exception type to handle unknown specified versions."""

    def __init__(self, value):
        UnknownBaseError.__init__(self, value)
        self.category = 'version'


def getASN1Tag(asn1Type):
    """Helper function for returning the base tag value of a given
    type from the pyasn1 package"""
    return asn1Type.baseTagSet.getBaseTag().asTuple()[2]

def stringToAlgorithmIdentifier(string):
    """Helper function that converts a description of an algorithm
    to a representation usable by the pyasn1 package"""
    algorithmIdentifier = rfc2459.AlgorithmIdentifier()
    algorithm = None
    if string == 'sha256WithRSAEncryption':
        algorithm = univ.ObjectIdentifier('1.2.840.113549.1.1.11')
    
    if algorithm == None:
        raise UnknownAlgorithmTypeError(string)
    algorithmIdentifier.setComponentByName('algorithm', algorithm)
    return algorithmIdentifier

def stringToCommonName(string):
    """Helper function for taking a string and building an x520 name
    representation usable by the pyasn1 package. Currently returns one
    RDN with one AVA consisting of a Common Name encoded as a
    UTF8String."""
    commonName = rfc2459.X520CommonName()
    commonName.setComponentByName('utf8String', string)
    ava = rfc2459.AttributeTypeAndValue()
    ava.setComponentByName('type', rfc2459.id_at_commonName)
    ava.setComponentByName('value', commonName)
    rdn = rfc2459.RelativeDistinguishedName()
    rdn.setComponentByPosition(0, ava)
    rdns = rfc2459.RDNSequence()
    rdns.setComponentByPosition(0, rdn)
    name = rfc2459.Name()
    name.setComponentByPosition(0, rdns)
    return name

def datetimeToTime(dt):
    """Takes a datetime object and returns an rfc2459.Time object with
    that time as its value as a GeneralizedTime"""
    time = rfc2459.Time()
    time.setComponentByName('generalTime', useful.GeneralizedTime(dt.strftime('%Y%m%d%H%M%SZ')))
    return time

class Certificate:
    """Utility class for reading a certificate specification and
    generating a signed x509 certificate"""

    def __init__(self, paramStream, now=datetime.datetime.utcnow()):
        self.versionValue = 2 
        self.signature = 'sha256WithRSAEncryption'
        self.issuer = 'Default Issuer'
        oneYear = datetime.timedelta(days=365)
        self.notBefore = now - oneYear
        self.notAfter = now + oneYear
        self.subject = 'Default Subject'
        self.signatureAlgorithm = 'sha256WithRSAEncryption'
        self.extensions = None
        self.subjectKey = pykey.RSAKey()
        self.issuerKey = pykey.RSAKey()
        self.decodeParams(paramStream)
        self.serialNumber = self.generateSerialNumber()

    def generateSerialNumber(self):
        """Generates a serial number for this certificate based on its
        contents. Intended to be reproducible for compatibility with
        the build system on OS X (see the comment above main, later in
        this file)."""
        hasher = hashlib.sha256()
        hasher.update(str(self.versionValue))
        hasher.update(self.signature)
        hasher.update(self.issuer)
        hasher.update(str(self.notBefore))
        hasher.update(str(self.notAfter))
        hasher.update(self.subject)
        hasher.update(self.signatureAlgorithm)
        if self.extensions:
            for extension in self.extensions:
                hasher.update(str(extension))
        serialBytes = [ord(c) for c in hasher.digest()[:20]]
        
        
        
        serialBytes[0] &= 0x7f
        
        
        
        serialBytes[0] |= 0x01
        
        serialBytes.insert(0, len(serialBytes))
        serialBytes.insert(0, getASN1Tag(univ.Integer))
        return ''.join(chr(b) for b in serialBytes)

    def decodeParams(self, paramStream):
        for line in paramStream.readlines():
            self.decodeParam(line.strip())

    def decodeParam(self, line):
        param = line.split(':')[0]
        value = ':'.join(line.split(':')[1:])
        if param == 'version':
            self.setVersion(value)
        elif param == 'subject':
            self.subject = value
        elif param == 'issuer':
            self.issuer = value
        elif param == 'extension':
            self.decodeExtension(value)
        elif param == 'issuerKey':
            self.setupKey('issuer', value)
        elif param == 'subjectKey':
            self.setupKey('subject', value)
        else:
            raise UnknownParameterTypeError(param)

    def setVersion(self, version):
        intVersion = int(version)
        if intVersion >= 1 and intVersion <= 4:
            self.versionValue = intVersion - 1
        else:
            raise UnknownVersionError(version)

    def decodeExtension(self, extension):
        extensionType = extension.split(':')[0]
        value = ':'.join(extension.split(':')[1:])
        if extensionType == 'basicConstraints':
            self.addBasicConstraints(value)
        elif extensionType == 'keyUsage':
            self.addKeyUsage(value)
        elif extensionType == 'extKeyUsage':
            self.addExtKeyUsage(value)
        elif extensionType == 'subjectAlternativeName':
            self.addSubjectAlternativeName(value)
        else:
            raise UnknownExtensionTypeError(extensionType)

    def setupKey(self, subjectOrIssuer, value):
        if subjectOrIssuer == 'subject':
            self.subjectKey = pykey.RSAKey(value)
        elif subjectOrIssuer == 'issuer':
            self.issuerKey = pykey.RSAKey(value)
        else:
            raise UnknownKeyTargetError(subjectOrIssuer)

    def addExtension(self, extensionType, extensionValue):
        if not self.extensions:
            self.extensions = []
        encapsulated = univ.OctetString(encoder.encode(extensionValue))
        extension = rfc2459.Extension()
        extension.setComponentByName('extnID', extensionType)
        extension.setComponentByName('extnValue', encapsulated)
        self.extensions.append(extension)

    def addBasicConstraints(self, basicConstraints):
        cA = basicConstraints.split(',')[0]
        pathLenConstraint = basicConstraints.split(',')[1]
        basicConstraintsExtension = rfc2459.BasicConstraints()
        basicConstraintsExtension.setComponentByName('cA', cA == 'cA')
        if pathLenConstraint:
            pathLenConstraintValue = \
                univ.Integer(int(pathLenConstraint)).subtype(
                    subtypeSpec=constraint.ValueRangeConstraint(0, 64))
            basicConstraintsExtension.setComponentByName('pathLenConstraint',
                                                         pathLenConstraintValue)
        self.addExtension(rfc2459.id_ce_basicConstraints, basicConstraintsExtension)

    def addKeyUsage(self, keyUsage):
        keyUsageExtension = rfc2459.KeyUsage(keyUsage)
        self.addExtension(rfc2459.id_ce_keyUsage, keyUsageExtension)

    def keyPurposeToOID(self, keyPurpose):
        if keyPurpose == 'serverAuth':
            
            
            return univ.ObjectIdentifier('1.3.6.1.5.5.7.3.1')
        if keyPurpose == 'clientAuth':
            return rfc2459.id_kp_clientAuth
        if keyPurpose == 'codeSigning':
            return rfc2459.id_kp_codeSigning
        if keyPurpose == 'emailProtection':
            return rfc2459.id_kp_emailProtection
        if keyPurpose == 'nsSGC':
            return univ.ObjectIdentifier('2.16.840.1.113730.4.1')
        if keyPurpose == 'OCSPSigning':
            return univ.ObjectIdentifier('1.3.6.1.5.5.7.3.9')
        if keyPurpose == 'timeStamping':
            return rfc2459.id_kp_timeStamping
        raise UnknownKeyPurposeTypeError(keyPurpose)

    def addExtKeyUsage(self, extKeyUsage):
        extKeyUsageExtension = rfc2459.ExtKeyUsageSyntax()
        count = 0
        for keyPurpose in extKeyUsage.split(','):
            extKeyUsageExtension.setComponentByPosition(count, self.keyPurposeToOID(keyPurpose))
            count += 1
        self.addExtension(rfc2459.id_ce_extKeyUsage, extKeyUsageExtension)

    def addSubjectAlternativeName(self, dNSNames):
        subjectAlternativeName = rfc2459.SubjectAltName()
        count = 0
        for dNSName in dNSNames.split(','):
            generalName = rfc2459.GeneralName()
            generalName.setComponentByName('dNSName', dNSName)
            subjectAlternativeName.setComponentByPosition(count, generalName)
            count += 1
        self.addExtension(rfc2459.id_ce_subjectAltName, subjectAlternativeName)

    def getVersion(self):
        return rfc2459.Version(self.versionValue).subtype(
            explicitTag=tag.Tag(tag.tagClassContext, tag.tagFormatSimple, 0))

    def getSerialNumber(self):
        return decoder.decode(self.serialNumber)[0]

    def getSignature(self):
        return stringToAlgorithmIdentifier(self.signature)

    def getIssuer(self):
        return stringToCommonName(self.issuer)

    def getValidity(self):
        validity = rfc2459.Validity()
        validity.setComponentByName('notBefore', self.getNotBefore())
        validity.setComponentByName('notAfter', self.getNotAfter())
        return validity

    def getNotBefore(self):
        return datetimeToTime(self.notBefore)

    def getNotAfter(self):
        return datetimeToTime(self.notAfter)

    def getSubject(self):
        return stringToCommonName(self.subject)

    def getSignatureAlgorithm(self):
        return stringToAlgorithmIdentifier(self.signature)

    def toDER(self):
        tbsCertificate = rfc2459.TBSCertificate()
        tbsCertificate.setComponentByName('version', self.getVersion())
        tbsCertificate.setComponentByName('serialNumber', self.getSerialNumber())
        tbsCertificate.setComponentByName('signature', self.getSignature())
        tbsCertificate.setComponentByName('issuer', self.getIssuer())
        tbsCertificate.setComponentByName('validity', self.getValidity())
        tbsCertificate.setComponentByName('subject', self.getSubject())
        tbsCertificate.setComponentByName('subjectPublicKeyInfo',
                                          self.subjectKey.asSubjectPublicKeyInfo())
        if self.extensions:
            extensions = rfc2459.Extensions().subtype(
                explicitTag=tag.Tag(tag.tagClassContext, tag.tagFormatSimple, 3))
            count = 0
            for extension in self.extensions:
                extensions.setComponentByPosition(count, extension)
                count += 1
            tbsCertificate.setComponentByName('extensions', extensions)
        certificate = rfc2459.Certificate()
        certificate.setComponentByName('tbsCertificate', tbsCertificate)
        certificate.setComponentByName('signatureAlgorithm', self.getSignatureAlgorithm())
        tbsDER = encoder.encode(tbsCertificate)
        certificate.setComponentByName('signatureValue', self.issuerKey.sign(tbsDER))
        return encoder.encode(certificate)

    def toPEM(self):
        output = '-----BEGIN CERTIFICATE-----'
        der = self.toDER()
        b64 = base64.b64encode(der)
        while b64:
            output += '\n' + b64[:64]
            b64 = b64[64:]
        output += '\n-----END CERTIFICATE-----'
        return output










def main(output, inputPath, buildIDPath):
    with open(buildIDPath) as buildidFile:
        buildid = buildidFile.read().strip()
    now = datetime.datetime.strptime(buildid, '%Y%m%d%H%M%S')
    with open(inputPath) as configStream:
        output.write(Certificate(configStream, now=now).toPEM())



if __name__ == '__main__':
    print Certificate(sys.stdin).toPEM()

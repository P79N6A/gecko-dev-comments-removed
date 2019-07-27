































versions = {
    'v1': 1,
    'v2': 2,
    'v3': 3,
    'v4': 4
}

basicConstraintsTypes = {
    'noBC': '',
    'BC-not-cA': 'extension:basicConstraints:,',
    'BC-cA': 'extension:basicConstraints:cA,'
}

def writeCertspec(issuer, subject, fields):
    filename = '%s_%s.pem.certspec' % (subject, issuer)
    if issuer == subject:
        filename = '%s.pem.certspec' % subject
    with open(filename, 'w') as f:
        f.write('issuer:%s\n' % issuer)
        f.write('subject:%s\n' % subject)
        for field in fields:
            if len(field) > 0:
                f.write('%s\n' % field)

keyUsage = 'extension:keyUsage:keyCertSign,cRLSign'
basicConstraintsCA = 'extension:basicConstraints:cA,'

writeCertspec('ca', 'ca', [keyUsage, basicConstraintsCA])

for versionStr, versionVal in versions.iteritems():
    
    versionText = 'version:%s' % versionVal
    for basicConstraintsType, basicConstraintsExtension in basicConstraintsTypes.iteritems():
        intermediateName = 'int-%s-%s' % (versionStr, basicConstraintsType)
        writeCertspec('ca', intermediateName,
                      [keyUsage, versionText, basicConstraintsExtension])
        writeCertspec(intermediateName, 'ee', [])

    
    versionText = 'version:%s' % versionVal
    for basicConstraintsType, basicConstraintsExtension in basicConstraintsTypes.iteritems():
        writeCertspec('ca', 'ee-%s-%s' % (versionStr, basicConstraintsType),
                      [versionText, basicConstraintsExtension])

    
    versionText = 'version:%s' % versionVal
    for basicConstraintsType, basicConstraintsExtension in basicConstraintsTypes.iteritems():
        selfSignedName = 'ss-%s-%s' % (versionStr, basicConstraintsType)
        writeCertspec(selfSignedName, selfSignedName,
                      [versionText, basicConstraintsExtension])

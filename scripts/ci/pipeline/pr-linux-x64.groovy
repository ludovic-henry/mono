
node('debian-9-amd64') {
    timestamps {
        stage ('Run') {
            chroot chrootName: 'debian-9-amd64-stable', command: 'scripts/ci/run-jenkins.sh', additionalPackages: 'xvfb xauth mono-devel git python wget bc build-essential libtool autoconf automake gettext iputils-ping cmake lsof'
        }
        stage ('NUnit') {
            step([
                $class: 'XUnitBuilder',
                testTimeMargin: '3000',
                thresholdMode: 1,
                thresholds: [
                    [$class: 'FailedThreshold', failureNewThreshold: '99', failureThreshold: '99', unstableNewThreshold: '0', unstableThreshold: '0'],
                    [$class: 'SkippedThreshold', failureNewThreshold: '2000', failureThreshold: '2000', unstableNewThreshold: '2000', unstableThreshold: '2000']],
                tools: [
                    [$class: 'NUnitJunitHudsonTestType', deleteOutputFiles: true, failIfNotNew: true, pattern: 'mcs/class/**/TestResult*.xml, mono/**/TestResult*.xml, runtime/**/TestResult*.xml, acceptance-tests/**/TestResult*.xml', skipNoTestFiles: true, stopProcessingIfError: true]]
            ])
        }
    }
}

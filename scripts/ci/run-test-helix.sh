#!/bin/bash -e

${TESTCMD} --label=compile-bcl-tests --timeout=40m make -i -w -C runtime -j4 test xunit-test
${TESTCMD} --label=package-for-helix --timeout=5m --fatal make -w -C runtime package-helix
rm -rf helix-tasks.zip helix-tasks
# TODO: reupload Microsoft.DotNet.Build.CloudTest to xamjenkinsarticats and package in .tar to avoid unzip dependency
wget -O helix-tasks.zip "https://dotnet.myget.org/F/dotnet-buildtools/api/v2/package/Microsoft.DotNet.Build.CloudTest/2.2.0-preview1-03013-03"
unzip helix-tasks.zip -d helix-tasks
tee <<'EOF' helix.proj
<Project ToolsVersion="14.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">

  <PropertyGroup>
    <HelixApiEndpoint>https://helix.dot.net/api/2018-03-14/jobs</HelixApiEndpoint>
    <HelixJobType>test/functional/cli/</HelixJobType>
    <HelixSource>pr/unspecified/</HelixSource>
    <HelixCorrelationInfoFileName>SubmittedHelixRuns.txt</HelixCorrelationInfoFileName>
    <CloudDropConnectionString>DefaultEndpointsProtocol=https;AccountName=helixstoragetest;AccountKey=$(CloudDropAccountKey);EndpointSuffix=core.windows.net</CloudDropConnectionString>
    <CloudResultsConnectionString>$(CloudDropConnectionString)</CloudResultsConnectionString>
    <ArchivesRoot>.</ArchivesRoot>
    <UseScriptRunner>true</UseScriptRunner>
  </PropertyGroup>

  <ItemGroup>
    <MonoTestAssemblies Include="net_4_x_corlib_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.Core_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.ComponentModel.Composition_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.Data_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.Drawing_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.Json_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.Numerics_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.Runtime.CompilerServices.Unsafe_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.Runtime.Serialization_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.Security_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.Threading.Tasks.Dataflow_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_System.Xml.Linq_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_Microsoft.CSharp_xunit-test.dll" />
    <MonoTestAssemblies Include="net_4_x_Mono.Profiler.Log_xunit-test.dll" />
  </ItemGroup>

  <Target Name="CreateMonoTestsBundle">
    <ZipFileCreateFromDirectory
        SourceDirectory="runtime/helix-payload"
        DestinationArchive="helix-tests.zip"
        OverwriteDestination="true" />
  </Target>

  <Target Name="PopulateHelixWorkItems" DependsOnTargets="CreateMonoTestsBundle">
    <ItemGroup>
      <HelixWorkItem Include="%(MonoTestAssemblies.Identity)">
        <WorkItemId>%(MonoTestAssemblies.Identity)</WorkItemId>
        <Command>mono-helix-wrapper.sh run-bcl-tests %(MonoTestAssemblies.Identity)</Command>
        <PayloadFile>helix-tests.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
    </ItemGroup>
  </Target>

  <Import Project="helix-tasks\build\CloudTest.Helix.targets" />

  <Target Name="RunCloudTest" DependsOnTargets="PopulateHelixWorkItems">
    <Message Text="Beginning Cloud Build!" />
    <CallTarget Targets="HelixCloudBuild" />
  </Target>

</Project>
EOF
    ${TESTCMD} --label=upload-to-helix --timeout=5m --fatal msbuild helix.proj -t:RunCloudTest -p:HelixApiAccessKey=${MONO_HELIX_API_KEY} -p:CloudDropAccountKey=${MONO_HELIX_CLOUDDROUP_ACCOUNTKEY} -p:TargetQueues=debian.9.amd64.open -p:BuildMoniker=${sha1} -p:HelixArchLabel=amd64 -p:HelixConfigLabel=Mainline
    HELIX_CORRELATION_ID=$(grep -Eo '[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}' SubmittedHelixRuns.txt) # TODO: improve
    tee <<EOF wait-helix-done.sh
#!/bin/sh
while ! wget -qO- "https://helix.dot.net/api/2018-03-14/jobs/${HELIX_CORRELATION_ID}/wait?access_token=${MONO_HELIX_API_KEY}"; do
    echo "Waiting for tests to finish..."
    sleep 30s
done
EOF
chmod +x wait-helix-done.sh
${TESTCMD} --label=wait-helix-done --timeout=300m ./wait-helix-done.sh

#!/bin/bash -e

${TESTCMD} --label=compile-mini-tests --timeout=5m --fatal make -w -C mono/mini -j ${CI_CPU_COUNT} compile-tests
${TESTCMD} --label=compile-runtime-tests --timeout=5m --fatal make -w -C mono/tests -j ${CI_CPU_COUNT} tests
${TESTCMD} --label=compile-profiler-tests --timeout=5m --fatal make -w -C mono/profiler -j ${CI_CPU_COUNT} compile-tests
${TESTCMD} --label=compile-bcl-tests --timeout=40m --fatal make -i -w -C runtime -j ${CI_CPU_COUNT} test xunit-test
${TESTCMD} --label=compile-mcs-tests --timeout=40m --fatal make -w -C mcs/tests -j ${CI_CPU_COUNT} test
${TESTCMD} --label=compile-mcs-errors-tests --timeout=40m --fatal make -w -C mcs/errors -j ${CI_CPU_COUNT} test
${TESTCMD} --label=package-for-helix --timeout=5m --fatal make -w package-helix
rm -rf helix-tasks.zip helix-tasks
# TODO: reupload Microsoft.DotNet.Build.CloudTest to xamjenkinsarticats and package in .tar to avoid unzip dependency
wget -O helix-tasks.zip "https://dotnet.myget.org/F/dotnet-buildtools/api/v2/package/Microsoft.DotNet.Build.CloudTest/2.2.0-preview1-03013-03"
unzip helix-tasks.zip -d helix-tasks
tee <<'EOF' helix.proj
<Project ToolsVersion="14.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">

  <PropertyGroup>
    <HelixApiEndpoint>https://helix.dot.net/api/2018-03-14/jobs</HelixApiEndpoint>
    <HelixJobType>test/mainline/</HelixJobType>
    <HelixCorrelationInfoFileName>SubmittedHelixRuns.txt</HelixCorrelationInfoFileName>
    <CloudDropConnectionString>DefaultEndpointsProtocol=https;AccountName=helixstoragetest;AccountKey=$(CloudDropAccountKey);EndpointSuffix=core.windows.net</CloudDropConnectionString>
    <CloudResultsConnectionString>$(CloudDropConnectionString)</CloudResultsConnectionString>
    <CloudResultsReadTokenValidDays>365</CloudResultsReadTokenValidDays>
    <ArchivesRoot>.</ArchivesRoot>
    <UseScriptRunner>true</UseScriptRunner>
  </PropertyGroup>

  <ItemGroup>
    <!-- xUnit tests -->
    <MonoXunitTestAssemblies Include="net_4_x_corlib_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Xml_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Xml.Linq_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Threading.Tasks.Dataflow_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Security_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Runtime.Serialization_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Runtime.CompilerServices.Unsafe_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Numerics_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Json_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Drawing_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Data_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.Core_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_System.ComponentModel.Composition_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_Mono.Profiler.Log_xunit-test.dll" />
    <MonoXunitTestAssemblies Include="net_4_x_Microsoft.CSharp_xunit-test.dll" />

    <!-- NUnit tests -->
    <MonoNUnitTestAssemblies Include="net_4_x_monodoc_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_corlib_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_WindowsBase_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_WebMatrix.Data_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Xml_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Xml.Linq_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Xaml_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Windows.Forms_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Windows.Forms.DataVisualization_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Web_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Web.Services_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Web.Routing_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Web.Extensions_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Web.DynamicData_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Web.Abstractions_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Transactions_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Threading.Tasks.Dataflow_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.ServiceProcess_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.ServiceModel_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.ServiceModel.Web_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.ServiceModel.Discovery_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Security_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Runtime.Serialization_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Runtime.Serialization.Formatters.Soap_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Runtime.Remoting_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Runtime.DurableInstancing_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Runtime.Caching_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Numerics_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Net.Http_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Messaging_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Json_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Json.Microsoft_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.IdentityModel_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.IO.Compression_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.IO.Compression.FileSystem_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Drawing_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.DirectoryServices_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Design_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Data_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Data.Services_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Data.OracleClient_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Data.Linq_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Data.DataSetExtensions_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Core_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.Configuration_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_System.ComponentModel.DataAnnotations_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Novell.Directory.Ldap_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.XBuild.Tasks_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Tasklets_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Security_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Runtime.Tests_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Posix_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Parallel_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Options_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Messaging_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Messaging.RabbitMQ_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Debugger.Soft_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Data.Tds_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.Data.Sqlite_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.CodeContracts_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.CSharp_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Mono.C5_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Microsoft.Build_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Microsoft.Build.Utilities_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Microsoft.Build.Tasks_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Microsoft.Build.Framework_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Microsoft.Build.Engine_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_I18N.West_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_I18N.Rare_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_I18N.Other_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_I18N.MidEast_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_I18N.CJK_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Cscompmgd_test.dll" />
    <MonoNUnitTestAssemblies Include="net_4_x_Commons.Xml.Relaxng_test.dll" />
  </ItemGroup>

  <ItemGroup>
    <HelixCorrelationPayloadFile Include="helix-payload.zip" />
  </ItemGroup>

  <Target Name="CreateMonoTestsBundle">
    <ZipFileCreateFromDirectory
        SourceDirectory="helix-payload"
        DestinationArchive="helix-payload.zip"
        OverwriteDestination="true" />
    <ZipFileCreateFromDirectory
        SourceDirectory="helix-wrapper"
        DestinationArchive="helix-wrapper.zip"
        OverwriteDestination="true" />
  </Target>

  <Target Name="PopulateHelixWorkItems" DependsOnTargets="CreateMonoTestsBundle">
    <ItemGroup>
      <HelixWorkItem Include="%(MonoXunitTestAssemblies.Identity)">
        <WorkItemId>%(MonoXunitTestAssemblies.Identity)</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-xunit %(MonoXunitTestAssemblies.Identity)</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
      <HelixWorkItem Include="%(MonoNUnitTestAssemblies.Identity)">
        <WorkItemId>%(MonoNUnitTestAssemblies.Identity)</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-nunit %(MonoNUnitTestAssemblies.Identity)</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
      <HelixWorkItem Include="mcs">
        <WorkItemId>mcs</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-mcs</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
      <HelixWorkItem Include="mcs-errors">
        <WorkItemId>mcs-errors</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-mcs-errors</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
      <HelixWorkItem Include="verify">
        <WorkItemId>verify</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-verify</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
      <HelixWorkItem Include="aot-test">
        <WorkItemId>aot-test</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-aot-test</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
      <HelixWorkItem Include="mini">
        <WorkItemId>mini</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-mini</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
      <HelixWorkItem Include="symbolicate">
        <WorkItemId>symbolicate</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-symbolicate</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
      <HelixWorkItem Include="csi">
        <WorkItemId>csi</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-csi</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
      <HelixWorkItem Include="profiler">
        <WorkItemId>profiler</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-profiler</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
      <HelixWorkItem Include="runtime">
        <WorkItemId>runtime</WorkItemId>
        <Command>mono-helix-wrapper.sh $HELIX_CORRELATION_PAYLOAD run-runtime</Command>
        <PayloadFile>helix-wrapper.zip</PayloadFile>
        <TimeoutInSeconds>300</TimeoutInSeconds>
      </HelixWorkItem>
    </ItemGroup>
  </Target>

  <Import Project="helix-tasks\build\CloudTest.Helix.targets" />

  <Target Name="RunCloudTest" DependsOnTargets="PopulateHelixWorkItems">
    <Message Text="Beginning Helix tests!" />
    <CallTarget Targets="HelixCloudBuild" />
  </Target>

</Project>
EOF
    helix_target_queues=debian.9.amd64.open
    helix_source=automated/mono/mono/$MONO_BRANCH/
    helix_build_moniker=$(git rev-parse HEAD)
    helix_config_label=mainline
    helix_creator=akoeplinger
    if [[ ${CI_TAGS} == *'-i386'*  ]]; then helix_arch_label=x86; fi
    if [[ ${CI_TAGS} == *'-amd64'* ]]; then helix_arch_label=x64; fi
    ${TESTCMD} --label=upload-to-helix --timeout=5m --fatal msbuild helix.proj -t:RunCloudTest -p:HelixApiAccessKey="${MONO_HELIX_API_KEY}" -p:CloudDropAccountKey="${MONO_HELIX_CLOUDDROUP_ACCOUNTKEY}" -p:TargetQueues="${helix_target_queues}" -p:BuildMoniker="${helix_build_moniker}" -p:HelixArchLabel="${helix_arch_label}" -p:HelixConfigLabel="${helix_config_label}" -p:HelixCreator="${helix_creator}" -p:HelixSource="${helix_source}"
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

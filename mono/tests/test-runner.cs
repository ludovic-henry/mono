//
// test-runner.cs
//
// Author:
//   Zoltan Varga (vargaz@gmail.com)
//
// Copyright (C) 2008 Novell, Inc (http://www.novell.com)
//
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//
using System;
using System.IO;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using System.Globalization;
using System.Xml;
using System.Text.RegularExpressions;
using Mono.Unix.Native;

//
// This is a simple test runner with support for parallel execution
//

public class TestRunner
{
	static void RegisterTests ()
	{
		// Tests.Register (...)
		Tests.Register ("bug-2907.cs");
		Tests.Register ("array-init.cs");
		Tests.Register ("arraylist.cs");
		Tests.Register ("assembly-load-remap.cs");
		Tests.Register ("assemblyresolve_event.cs");
		Tests.Register ("assemblyresolve_event3.cs");
		Tests.Register ("assemblyresolve_event4.cs");
		Tests.Register ("checked.cs");
		Tests.Register ("char-isnumber.cs");
		Tests.Register ("create-instance.cs");
		Tests.Register ("field-layout.cs");
		Tests.Register ("pack-layout.cs");
		Tests.Register ("pack-bug.cs");
		Tests.Register ("hash-table.cs");
		Tests.Register ("test-ops.cs");
		Tests.Register ("obj.cs");
		Tests.Register ("test-dup-mp.cs");
		Tests.Register ("string.cs");
		Tests.Register ("stringbuilder.cs");
		Tests.Register ("switch.cs");
		Tests.Register ("outparm.cs");
		Tests.Register ("delegate.cs");
		Tests.Register ("bitconverter.cs");
		Tests.Register ("exception.cs");
		Tests.Register ("exception2.cs");
		Tests.Register ("exception3.cs");
		Tests.Register ("exception4.cs");
		Tests.Register ("exception5.cs");
		Tests.Register ("exception6.cs");
		Tests.Register ("exception7.cs");
		Tests.Register ("exception8.cs");
		Tests.Register ("exception10.cs");
		Tests.Register ("exception11.cs");
		Tests.Register ("exception12.cs");
		Tests.Register ("exception13.cs");
		Tests.Register ("exception14.cs");
		Tests.Register ("exception15.cs");
		Tests.Register ("exception16.cs");
		Tests.Register ("exception17.cs");
		Tests.Register ("exception18.cs");
		Tests.Register ("typeload-unaligned.cs");
		Tests.Register ("struct.cs");
		Tests.Register ("valuetype-gettype.cs");
		Tests.Register ("typeof-ptr.cs");
		Tests.Register ("static-constructor.cs");
		Tests.Register ("pinvoke.cs");
		Tests.Register ("pinvoke2.cs");
		Tests.Register ("pinvoke3.cs");
		Tests.Register ("pinvoke11.cs");
		Tests.Register ("pinvoke13.cs");
		Tests.Register ("pinvoke17.cs");
		Tests.Register ("invoke.cs");
		Tests.Register ("invoke2.cs");
		Tests.Register ("runtime-invoke.cs");
		Tests.Register ("invoke-string-ctors.cs");
		Tests.Register ("reinit.cs");
		Tests.Register ("box.cs");
		Tests.Register ("array.cs");
		Tests.Register ("enum.cs");
		Tests.Register ("enum2.cs");
		Tests.Register ("enum-intrins.cs");
		Tests.Register ("property.cs");
		Tests.Register ("enumcast.cs");
		Tests.Register ("assignable-tests.cs");
		Tests.Register ("array-cast.cs");
		Tests.Register ("array-subtype-attr.cs");
		Tests.Register ("cattr-compile.cs");
		Tests.Register ("cattr-field.cs");
		Tests.Register ("cattr-object.cs");
		Tests.Register ("custom-attr.cs");
		Tests.Register ("double-cast.cs");
		Tests.Register ("newobj-valuetype.cs");
		Tests.Register ("arraylist-clone.cs");
		Tests.Register ("setenv.cs");
		Tests.Register ("vtype.cs");
		Tests.Register ("isvaluetype.cs");
		Tests.Register ("iface6.cs");
		Tests.Register ("iface7.cs");
		Tests.Register ("ipaddress.cs");
		Tests.Register ("array-vt.cs");
		Tests.Register ("interface1.cs");
		Tests.Register ("reflection-enum.cs");
		Tests.Register ("reflection-prop.cs");
		Tests.Register ("reflection4.cs");
		Tests.Register ("reflection5.cs");
		Tests.Register ("reflection-const-field.cs");
		Tests.Register ("many-locals.cs");
		Tests.Register ("string-compare.cs");
		Tests.Register ("test-prime.cs");
		Tests.Register ("test-tls.cs");
		Tests.Register ("params.cs");
		Tests.Register ("reflection.cs");
		Tests.Register ("interface.cs");
		Tests.Register ("iface.cs");
		Tests.Register ("iface2.cs");
		Tests.Register ("iface3.cs");
		Tests.Register ("iface4.cs");
		Tests.Register ("iface-large.cs");
		Tests.Register ("virtual-method.cs");
		Tests.Register ("intptrcast.cs");
		Tests.Register ("indexer.cs");
		Tests.Register ("stream.cs");
		Tests.Register ("console.cs");
		Tests.Register ("shift.cs");
		Tests.Register ("jit-int.cs");
		Tests.Register ("jit-uint.cs");
		Tests.Register ("jit-long.cs");
		Tests.Register ("long.cs");
		Tests.Register ("jit-ulong.cs");
		Tests.Register ("jit-float.cs");
		Tests.Register ("pop.cs");
		Tests.Register ("time.cs");
		Tests.Register ("appdomain.cs");
		Tests.Register ("appdomain1.cs");
		Tests.Register ("appdomain2.cs");
		Tests.Register ("appdomain-client.cs");
		Tests.Register ("appdomain-unload.cs");
		Tests.Register ("appdomain-async-invoke.cs");
		Tests.Register ("loader.cs");
		Tests.Register ("pointer.cs");
		Tests.Register ("hashcode.cs");
		Tests.Register ("delegate1.cs");
		Tests.Register ("delegate2.cs");
		Tests.Register ("delegate3.cs");
		Tests.Register ("delegate5.cs");
		Tests.Register ("delegate6.cs");
		Tests.Register ("delegate7.cs");
		Tests.Register ("delegate8.cs");
		Tests.Register ("delegate9.cs");
		Tests.Register ("delegate10.cs");
		Tests.Register ("delegate11.cs");
		Tests.Register ("delegate12.cs");
		Tests.Register ("delegate13.cs");
		Tests.Register ("remoting1.cs");
		Tests.Register ("remoting2.cs");
		Tests.Register ("remoting3.cs");
		Tests.Register ("remoting4.cs");
		Tests.Register ("remoting5.cs");
		Tests.Register ("largeexp.cs");
		Tests.Register ("largeexp2.cs");
		Tests.Register ("marshalbyref1.cs");
		Tests.Register ("static-ctor.cs");
		Tests.Register ("inctest.cs");
		Tests.Register ("bound.cs");
		Tests.Register ("array-invoke.cs");
		Tests.Register ("test-arr.cs");
		Tests.Register ("decimal.cs");
		Tests.Register ("decimal-array.cs");
		Tests.Register ("marshal.cs");
		Tests.Register ("marshal1.cs");
		Tests.Register ("marshal2.cs");
		Tests.Register ("marshal3.cs");
		Tests.Register ("marshal5.cs");
		Tests.Register ("marshal6.cs");
		Tests.Register ("marshal7.cs");
		Tests.Register ("marshal8.cs");
		Tests.Register ("marshal9.cs");
		Tests.Register ("marshalbool.cs");
		Tests.Register ("marshal-valuetypes.cs");
		Tests.Register ("test-byval-in-struct.cs");
		Tests.Register ("thread.cs");
		Tests.Register ("thread5.cs");
		Tests.Register ("thread6.cs");
		Tests.Register ("thread-static.cs");
		Tests.Register ("thread-static-init.cs");
		Tests.Register ("context-static.cs");
		Tests.Register ("float-pop.cs");
		Tests.Register ("interfacecast.cs");
		Tests.Register ("array3.cs");
		Tests.Register ("classinit.cs");
		Tests.Register ("classinit2.cs");
		Tests.Register ("classinit3.cs");
		Tests.Register ("synchronized.cs");
		Tests.Register ("async_read.cs");
		Tests.Register ("threadpool.cs");
		Tests.Register ("threadpool1.cs");
		Tests.Register ("threadpool-exceptions1.cs");
		Tests.Register ("threadpool-exceptions2.cs");
		Tests.Register ("threadpool-exceptions3.cs");
		Tests.Register ("threadpool-exceptions4.cs");
		Tests.Register ("threadpool-exceptions5.cs");
		Tests.Register ("threadpool-exceptions6.cs");
		Tests.Register ("threadpool-exceptions7.cs");
		Tests.Register ("base-definition.cs");
		Tests.Register ("bug-27420.cs");
		Tests.Register ("bug-47295.cs");
		Tests.Register ("bug-46781.cs");
		Tests.Register ("bug-48015.cs");
		Tests.Register ("bug-42136.cs");
		Tests.Register ("bug-59286.cs");
		Tests.Register ("bug-70561.cs");
		Tests.Register ("bug-78311.cs");
		Tests.Register ("bug-78653.cs");
		Tests.Register ("bug-78656.cs");
		Tests.Register ("bug-77127.cs");
		Tests.Register ("bug-323114.cs");
		Tests.Register ("bug-Xamarin-5278.cs");
		Tests.Register ("interlocked.cs");
		Tests.Register ("cross-domain.cs");
		Tests.Register ("appdomain-exit.cs");
		Tests.Register ("delegate-async-exit.cs");
		Tests.Register ("delegate-delegate-exit.cs");
		Tests.Register ("delegate-exit.cs");
		Tests.Register ("finalizer-abort.cs");
		Tests.Register ("finalizer-exception.cs");
		Tests.Register ("finalizer-exit.cs");
		Tests.Register ("finalizer-thread.cs");
		Tests.Register ("main-exit.cs");
		Tests.Register ("main-returns-abort-resetabort.cs");
		Tests.Register ("main-returns-background-abort-resetabort.cs");
		Tests.Register ("main-returns-background-resetabort.cs");
		Tests.Register ("main-returns-background.cs");
		Tests.Register ("main-returns-background-change.cs");
		Tests.Register ("main-returns.cs");
		Tests.Register ("subthread-exit.cs");
		Tests.Register ("desweak.cs");
		Tests.Register ("cominterop.cs");
		Tests.Register ("exists.cs");
		Tests.Register ("handleref.cs");
		Tests.Register ("transparentproxy.cs");
		Tests.Register ("dbnull-missing.cs");
		Tests.Register ("test-type-ctor.cs");
		Tests.Register ("soft-float-tests.cs");
		Tests.Register ("thread-exit.cs");
		Tests.Register ("finalize-parent.cs");
		Tests.Register ("assemblyresolve_event2.2.cs");
		Tests.Register ("interlocked-2.2.cs");
		Tests.Register ("pinvoke-2.2.cs");
		Tests.Register ("bug-78431.2.cs");
		Tests.Register ("bug-79684.2.cs");
		Tests.Register ("catch-generics.2.cs");
		Tests.Register ("event-get.2.cs");
		Tests.Register ("safehandle.2.cs");
		Tests.Register ("stackframes-async.2.cs");
		Tests.Register ("module-cctor-loader.2.cs");
		Tests.Register ("generics-invoke-byref.2.cs");
		Tests.Register ("generic-signature-compare.2.cs");
		Tests.Register ("generics-sharing.2.cs");
		Tests.Register ("shared-generic-methods.2.cs");
		Tests.Register ("shared-generic-synchronized.2.cs");
		Tests.Register ("generic-inlining.2.cs");
		Tests.Register ("generic-initobj.2.cs");
		Tests.Register ("generic-delegate.2.cs");
		Tests.Register ("generic-sizeof.2.cs");
		Tests.Register ("generic-virtual.2.cs");
		Tests.Register ("generic-interface-methods.2.cs");
		Tests.Register ("generic-array-type.2.cs");
		Tests.Register ("generic-method-patching.2.cs");
		Tests.Register ("generic-static-methods.2.cs");
		Tests.Register ("generic-null-call.2.cs");
		Tests.Register ("generic-special.2.cs");
		Tests.Register ("generic-exceptions.2.cs");
		Tests.Register ("generic-virtual2.2.cs");
		Tests.Register ("generic-valuetype-interface.2.cs");
		Tests.Register ("generic-getgenericarguments.2.cs");
		Tests.Register ("generic-type-builder.2.cs");
		Tests.Register ("generic-synchronized.2.cs");
		Tests.Register ("generic-delegate-ctor.2.cs");
		Tests.Register ("generic-array-iface-set.2.cs");
		Tests.Register ("generic-typedef.2.cs");
		Tests.Register ("generic-marshalbyref.2.cs");
		Tests.Register ("generic-xdomain.2.cs");
		Tests.Register ("dynamic-generic-size.cs");
		Tests.Register ("bug-431413.2.cs");
		Tests.Register ("bug-459285.2.cs");
		Tests.Register ("generic-virtual-invoke.2.cs");
		Tests.Register ("bug-461198.2.cs");
		Tests.Register ("generic-sealed-virtual.2.cs");
		Tests.Register ("generic-system-arrays.2.cs");
		Tests.Register ("generic-stack-traces.2.cs");
		Tests.Register ("generic-stack-traces2.2.cs");
		Tests.Register ("bug-472600.2.cs");
		Tests.Register ("recursive-generics.2.cs");
		Tests.Register ("bug-473482.2.cs");
		Tests.Register ("bug-473999.2.cs");
		Tests.Register ("bug-479763.2.cs");
		Tests.Register ("bug-616463.cs");
		Tests.Register ("bug-80392.2.cs");
		Tests.Register ("dynamic-method-access.2.cs");
		Tests.Register ("dynamic-method-finalize.2.cs");
		Tests.Register ("dynamic-method-stack-traces.cs");
		Tests.Register ("bug-82194.2.cs");
		Tests.Register ("anonarray.2.cs");
		Tests.Register ("ienumerator-interfaces.2.cs");
		Tests.Register ("array-enumerator-ifaces.2.cs");
		Tests.Register ("generic_type_definition_encoding.2.cs");
		Tests.Register ("generic_type_definition.2.cs");
		Tests.Register ("bug-333798.2.cs");
		Tests.Register ("bug-333798-tb.2.cs");
		Tests.Register ("bug-335131.2.cs");
		Tests.Register ("bug-322722_patch_bx.2.cs");
		Tests.Register ("bug-348522.2.cs");
		Tests.Register ("bug-340662_bug.cs");
		Tests.Register ("bug-322722_dyn_method_throw.2.cs");
		Tests.Register ("bug-389886-2.cs");
		Tests.Register ("bug-325283.2.cs");
		Tests.Register ("thunks.cs");
		Tests.Register ("winx64structs.cs");
		Tests.Register ("bug-349190.2.cs");
		Tests.Register ("nullable_boxing.2.cs");
		Tests.Register ("valuetype-equals.cs");
		Tests.Register ("custom-modifiers.2.cs");
		Tests.Register ("bug-382986.cs");
		Tests.Register ("test-inline-call-stack.cs");
		Tests.Register ("bug-324535.cs");
		Tests.Register ("modules.cs");
		Tests.Register ("bug-81673.cs");
		Tests.Register ("bug-36848.cs");
		Tests.Register ("bug-81691.cs");
		Tests.Register ("bug-80307.cs");
		Tests.Register ("bug-415577.cs");
		Tests.Register ("filter-stack.cs");
		Tests.Register ("vararg2.cs");
		Tests.Register ("bug-389886-sre-generic-interface-instances.cs");
		Tests.Register ("bug-461867.cs");
		Tests.Register ("bug-461941.cs");
		Tests.Register ("bug-461261.cs");
		Tests.Register ("bug-400716.cs");
		Tests.Register ("bug-462592.cs");
		Tests.Register ("bug-459094.cs");
		Tests.Register ("generic-unloading.2.cs");
		Tests.Register ("generic-unloading-sub.2.cs");
		Tests.Register ("bug-467456.cs");
		Tests.Register ("appdomain-unload-callback.cs");
		Tests.Register ("bug-508538.cs");
		Tests.Register ("bug-472692.2.cs");
		Tests.Register ("gchandles.cs");
		Tests.Register ("interlocked-3.cs");
		Tests.Register ("interlocked-4.2.cs");
		Tests.Register ("appdomain-thread-abort.cs");
		Tests.Register ("xdomain-threads.cs");
		Tests.Register ("w32message.cs");
		Tests.Register ("bug-544446.cs");
		Tests.Register ("gc-altstack.cs");
		Tests.Register ("large-gc-bitmap.cs");
		Tests.Register ("bug-561239.cs");
		Tests.Register ("bug-562150.cs");
		Tests.Register ("bug-575941.cs");
		Tests.Register ("bug-599469.cs");
		Tests.Register ("bug-389886-3.cs");
		Tests.Register ("monitor.cs");
		Tests.Register ("monitor-resurrection.cs");
		Tests.Register ("monitor-wait-abort.cs");
		Tests.Register ("monitor-abort.cs");
		Tests.Register ("dynamic-method-resurrection.cs");
		Tests.Register ("bug-666008.cs");
		Tests.Register ("bug-685908.cs");
		Tests.Register ("sgen-long-vtype.cs");
		Tests.Register ("delegate-invoke.cs");
		Tests.Register ("bug-696593.cs");
		Tests.Register ("bug-705140.cs");
		Tests.Register ("bug-1147.cs");
		Tests.Register ("mono-path.cs");
		Tests.Register ("bug-bxc-795.cs");
		Tests.Register ("bug-3903.cs");
		Tests.Register ("async-with-cb-throws.cs");
		Tests.Register ("appdomain-unload-doesnot-raise-pending-events.cs");
		Tests.Register ("bug-6148.cs");
		Tests.Register ("assembly_append_ordering.cs");
		Tests.Register ("bug-10127.cs");
		Tests.Register ("bug-18026.cs");
		Tests.Register ("allow-synchronous-major.cs");
		Tests.Register ("unload-appdomain-on-shutdown.cs");
		Tests.Register ("block_guard_restore_aligment_on_exit.cs");
		Tests.Register ("thread_static_gc_layout.cs");
		Tests.Register ("sleep.cs");
		Tests.Register ("bug-27147.cs");
		Tests.Register ("bug-30085.cs");
		Tests.Register ("bug-17537.cs");
		Tests.Register ("pinvoke_ppcc.cs");
		Tests.Register ("pinvoke_ppcs.cs");
		Tests.Register ("pinvoke_ppci.cs");
		Tests.Register ("pinvoke_ppcf.cs");
		Tests.Register ("pinvoke_ppcd.cs");
	}

	const string TEST_TIME_FORMAT = "mm\\:ss\\.fff";
	const string ENV_TIMEOUT = "TEST_DRIVER_TIMEOUT_SEC";

	public static int Main (String[] args)
	{
		// Defaults
		int concurrency = 1;
		int timeout = 2 * 60; // in seconds
		int expectedExitCode = 0;
		string testsuiteName = null;
		string inputFile = null;

		// FIXME: Add support for runtime arguments + env variables

		string runtime = "mono";
		var opt_sets = null;

		// Process options
		int i = 0;
		while (i < args.Length) {
			if (args [i].StartsWith ("-")) {
				if (args [i] == "-M") {
					/* generate make output */
					Environment.Exit (0);
				} else if (args [i] == "-j") {
					if (i + 1 >= args.Length) {
						Console.WriteLine ("Missing argument to -j command line option.");
						return 1;
					}
					if (args [i + 1] == "a")
						concurrency = Environment.ProcessorCount;
					else
						concurrency = Int32.Parse (args [i + 1]);
					i += 2;
				} else if (args [i] == "--timeout") {
					if (i + 1 >= args.Length) {
						Console.WriteLine ("Missing argument to --timeout command line option.");
						return 1;
					}
					timeout = Int32.Parse (args [i + 1]);
					i += 2;
				} else if (args [i] == "--runtime") {
					if (i + 1 >= args.Length) {
						Console.WriteLine ("Missing argument to --runtime command line option.");
						return 1;
					}
					runtime = args [i + 1];
					i += 2;
				} else if (args [i] == "--opt-sets") {
					if (i + 1 >= args.Length) {
						Console.WriteLine ("Missing argument to --opt-sets command line option.");
						return 1;
					}
					if (opt_sets == null)
						opt_sets = new List<string> ();
					foreach (var s in args [i + 1].Split ())
						opt_sets.Add (s);
					i += 2;
				} else if (args [i] == "--expected-exit-code") {
					if (i + 1 >= args.Length) {
						Console.WriteLine ("Missing argument to --expected-exit-code command line option.");
						return 1;
					}
					expectedExitCode = Int32.Parse (args [i + 1]);
					i += 2;
				} else if (args [i] == "--testsuite-name") {
					if (i + 1 >= args.Length) {
						Console.WriteLine ("Missing argument to --testsuite-name command line option.");
						return 1;
					}
					testsuiteName = args [i + 1];
					i += 2;
				} else if (args [i] == "--input-file") {
					if (i + 1 >= args.Length) {
						Console.WriteLine ("Missing argument to --input-file command line option.");
						return 1;
					}
					inputFile = args [i + 1];
					i += 2;
				} else {
					Console.WriteLine ("Unknown command line option: '" + args [i] + "'.");
					return 1;
				}
			} else {
				break;
			}
		}

		if (String.IsNullOrEmpty (testsuiteName)) {
			Console.WriteLine ("Missing the required --testsuite-name command line option.");
			return 1;
		}

		if (String.IsNullOrEmpty (inputFile)) {
			RegisterTests ();
		} else {
			foreach (string o_flag in opt_sets != null ? opt_sets : new List<string> () { null })
				foreach (string filename in File.ReadAllLines (inputFile))
					Tests.Register (filename, o_flag: o_flag, expectedExitCode: expectedExitCode, timeout: timeout);
		}

		Console.WriteLine ("Running tests: ");

		TimeSpan wallClockTime;
		List<Test> passed, failed, timedout;

		Tests.RunAll (runtime, concurrency, out passed, out failed, out timedout, out wallClockTime);

		int npassed = passed.Count;
		int nfailed = failed.Count;
		int ntimedout = timedout.Count;

		XmlWriterSettings xmlWriterSettings = new XmlWriterSettings ();
		xmlWriterSettings.NewLineOnAttributes = true;
		xmlWriterSettings.Indent = true;
		using (XmlWriter writer = XmlWriter.Create (String.Format ("TestResult-{0}.xml", testsuiteName), xmlWriterSettings)) {
			// <?xml version="1.0" encoding="utf-8" standalone="no"?>
			writer.WriteStartDocument ();
			// <!--This file represents the results of running a test suite-->
			writer.WriteComment ("This file represents the results of running a test suite");
			// <test-results name="/home/charlie/Dev/NUnit/nunit-2.5/work/src/bin/Debug/tests/mock-assembly.dll" total="21" errors="1" failures="1" not-run="7" inconclusive="1" ignored="4" skipped="0" invalid="3" date="2010-10-18" time="13:23:35">
			writer.WriteStartElement ("test-results");
			writer.WriteAttributeString ("name", String.Format ("{0}-tests.dummy", testsuiteName));
			writer.WriteAttributeString ("total", (npassed + nfailed + ntimedout).ToString());
			writer.WriteAttributeString ("failures", (nfailed + ntimedout).ToString());
			writer.WriteAttributeString ("not-run", "0");
			writer.WriteAttributeString ("date", DateTime.Now.ToString ("yyyy-MM-dd"));
			writer.WriteAttributeString ("time", DateTime.Now.ToString ("HH:mm:ss"));
			//   <environment nunit-version="2.4.8.0" clr-version="4.0.30319.17020" os-version="Unix 3.13.0.45" platform="Unix" cwd="/home/directhex/Projects/mono/mcs/class/corlib" machine-name="marceline" user="directhex" user-domain="marceline" />
			writer.WriteStartElement ("environment");
			writer.WriteAttributeString ("nunit-version", "2.4.8.0" );
			writer.WriteAttributeString ("clr-version", Environment.Version.ToString() );
			writer.WriteAttributeString ("os-version", Environment.OSVersion.ToString() );
			writer.WriteAttributeString ("platform", Environment.OSVersion.Platform.ToString() );
			writer.WriteAttributeString ("cwd", Environment.CurrentDirectory );
			writer.WriteAttributeString ("machine-name", Environment.MachineName );
			writer.WriteAttributeString ("user", Environment.UserName );
			writer.WriteAttributeString ("user-domain", Environment.UserDomainName );
			writer.WriteEndElement ();
			//   <culture-info current-culture="en-GB" current-uiculture="en-GB" />
			writer.WriteStartElement ("culture-info");
			writer.WriteAttributeString ("current-culture", CultureInfo.CurrentCulture.Name );
			writer.WriteAttributeString ("current-uiculture", CultureInfo.CurrentUICulture.Name );
			writer.WriteEndElement ();
			//   <test-suite name="corlib_test_net_4_5.dll" success="True" time="114.318" asserts="0">
			writer.WriteStartElement ("test-suite");
			writer.WriteAttributeString ("name", String.Format ("{0}-tests.dummy", testsuiteName));
			writer.WriteAttributeString ("success", (nfailed + ntimedout == 0).ToString());
			writer.WriteAttributeString ("time", wallClockTime.Seconds.ToString());
			writer.WriteAttributeString ("asserts", (nfailed + ntimedout).ToString());
			//     <results>
			writer.WriteStartElement ("results");
			//       <test-suite name="MonoTests" success="True" time="114.318" asserts="0">
			writer.WriteStartElement ("test-suite");
			writer.WriteAttributeString ("name","MonoTests");
			writer.WriteAttributeString ("success", (nfailed + ntimedout == 0).ToString());
			writer.WriteAttributeString ("time", wallClockTime.Seconds.ToString());
			writer.WriteAttributeString ("asserts", (nfailed + ntimedout).ToString());
			//         <results>
			writer.WriteStartElement ("results");
			//           <test-suite name="MonoTests" success="True" time="114.318" asserts="0">
			writer.WriteStartElement ("test-suite");
			writer.WriteAttributeString ("name", testsuiteName);
			writer.WriteAttributeString ("success", (nfailed + ntimedout == 0).ToString());
			writer.WriteAttributeString ("time", wallClockTime.Seconds.ToString());
			writer.WriteAttributeString ("asserts", (nfailed + ntimedout).ToString());
			//             <results>
			writer.WriteStartElement ("results");
			// Dump all passing tests first
			foreach (Test test in passed) {
				// <test-case name="MonoTests.Microsoft.Win32.RegistryKeyTest.bug79051" executed="True" success="True" time="0.063" asserts="0" />
				writer.WriteStartElement ("test-case");
				writer.WriteAttributeString ("name", String.Format ("MonoTests.{0}.{1}", testsuiteName, test.FileName));
				writer.WriteAttributeString ("executed", "True");
				writer.WriteAttributeString ("success", "True");
				writer.WriteAttributeString ("time", "0");
				writer.WriteAttributeString ("asserts", "0");
				writer.WriteEndElement ();
			}
			// Now dump all failing tests
			foreach (Test test in failed) {
				// <test-case name="MonoTests.Microsoft.Win32.RegistryKeyTest.bug79051" executed="True" success="True" time="0.063" asserts="0" />
				writer.WriteStartElement ("test-case");
				writer.WriteAttributeString ("name", String.Format ("MonoTests.{0}.{1}", testsuiteName, test.FileName));
				writer.WriteAttributeString ("executed", "True");
				writer.WriteAttributeString ("success", "False");
				writer.WriteAttributeString ("time", "0");
				writer.WriteAttributeString ("asserts", "1");
				writer.WriteStartElement ("failure");
				writer.WriteStartElement ("message");
				writer.WriteCData (DumpPseudoTrace (test.StandardOutputFilename));
				writer.WriteEndElement ();
				writer.WriteStartElement ("stack-trace");
				writer.WriteCData (DumpPseudoTrace (test.StandardErrorFilename));
				writer.WriteEndElement ();
				writer.WriteEndElement ();
				writer.WriteEndElement ();
			}
			// Then dump all timing out tests
			foreach (Test test in timedout) {
				// <test-case name="MonoTests.Microsoft.Win32.RegistryKeyTest.bug79051" executed="True" success="True" time="0.063" asserts="0" />
				writer.WriteStartElement ("test-case");
				writer.WriteAttributeString ("name", String.Format ("MonoTests.{0}.{1}_timedout", testsuiteName, test.FileName));
				writer.WriteAttributeString ("executed", "True");
				writer.WriteAttributeString ("success", "False");
				writer.WriteAttributeString ("time", "0");
				writer.WriteAttributeString ("asserts", "1");
				writer.WriteStartElement ("failure");
				writer.WriteStartElement ("message");
				writer.WriteCData (DumpPseudoTrace (test.StandardOutputFilename));
				writer.WriteEndElement ();
				writer.WriteStartElement ("stack-trace");
				writer.WriteCData (DumpPseudoTrace (test.StandardErrorFilename));
				writer.WriteEndElement ();
				writer.WriteEndElement ();
				writer.WriteEndElement ();
			}
			//             </results>
			writer.WriteEndElement ();
			//           </test-suite>
			writer.WriteEndElement ();
			//         </results>
			writer.WriteEndElement ();
			//       </test-suite>
			writer.WriteEndElement ();
			//     </results>
			writer.WriteEndElement ();
			//   </test-suite>
			writer.WriteEndElement ();
			// </test-results>
			writer.WriteEndElement ();
			writer.WriteEndDocument ();
		}

		Console.WriteLine ();
		Console.WriteLine ("Time: {0}", test_time.ToString (TEST_TIME_FORMAT));
		Console.WriteLine ();
		Console.WriteLine ("{0,4} test(s) passed", npassed);
		Console.WriteLine ("{0,4} test(s) failed", nfailed);
		Console.WriteLine ("{0,4} test(s) timed out", ntimedout);

		if (nfailed > 0) {
			Console.WriteLine ();
			Console.WriteLine ("Failed test(s):");
			foreach (Test test in failed) {
				Console.WriteLine ();
				Console.WriteLine (test.Filename);
				DumpFile (test.StandardOutputFilename);
				DumpFile (test.StandardErrorFilename);
			}
		}

		if (ntimedout > 0) {
			Console.WriteLine ();
			Console.WriteLine ("Timed out test(s):");
			foreach (Test test in timedout) {
				Console.WriteLine ();
				Console.WriteLine (test.Filename);
				DumpFile (test.StandardOutputFilename);
				DumpFile (test.StandardErrorFilename);
			}
		}

		return (ntimedout == 0 && nfailed == 0) ? 0 : 1;
	}
	
	static void DumpFile (string filename) {
		if (File.Exists (filename)) {
			Console.WriteLine ("=============== {0} ===============", filename);
			Console.WriteLine (File.ReadAllText (filename));
			Console.WriteLine ("=============== EOF ===============");
		}
	}

	static string DumpPseudoTrace (string filename) {
		if (File.Exists (filename))
			return FilterInvalidXmlChars (File.ReadAllText (filename));
		else
			return string.Empty;
	}

	static string FilterInvalidXmlChars (string text) {
		// Spec at http://www.w3.org/TR/2008/REC-xml-20081126/#charsets says only the following chars are valid in XML:
		// Char ::= #x9 | #xA | #xD | [#x20-#xD7FF] | [#xE000-#xFFFD] | [#x10000-#x10FFFF]	/* any Unicode character, excluding the surrogate blocks, FFFE, and FFFF. */
		return Regex.Replace (text, @"[^\x09\x0A\x0D\x20-\uD7FF\uE000-\uFFFD\u10000-\u10FFFF]", "");
	}

	static class Tests
	{
		static object monitor = new object ();

		static List<Test> tests = new List<Test> ();

		public static void Register (string filename, string o_flag = null, int expectedExitCode = 0, int timeout = 120)
		{
			tests.Add (new Test (filename) {
				OFlag = o_flag,
				ExpectedExitCode = expectedExitCode,
				Timeout = timeout,
			});
		}

		public static void RunAll (string runtime, int concurrency, out List<Test> passed, out List<Test> failed, out List<Test> timedout, out TimeSpan wallClockTime)
		{
			passed = new List<Test> ();
			failed = new List<Test> ();
			timedout = new List<Test> ();

			Queue<Test> testsQueue = new Queue<Test> (tests);

			/* compute the max length of test names, to have an optimal output width */
			int output_width = -1;
			foreach (Test test in tests) {
				if (test.Filename.Length > output_width)
					output_width = Math.Min (120, test.Filename.Length);
			}

			Thread[] threads = new Thread [concurrency];

			for (int i = 0; i < concurrency; ++i) {
				threads [i] = new Thread (() => {
					while (true) {
						Test test;

						lock (monitor) {
							if (testsQueue.Count == 0)
								break;
							test = testsQueue.Dequeue ();
						}

						TextWriter output = new StringWriter ();

						Status status = test.Run (runtime, output, output_width);

						lock (monitor) {
							switch (status) {
							case Status.Passed:
								passed.Add (test);
								break;
							case Status.Failed:
								failed.Add (test);
								break;
							case Status.TimedOut:
								timedout.Add (test);
								break;
							}

							Console.WriteLine (output.ToString ());
						}
					}
				});
			}

			DateTime start = DateTime.UtcNow;

			for (int i = 0; i < concurrency; ++i)
				threads [i].Start ();
			for (int i = 0; i < concurrency; ++i)
				threads [i].Join ();

			wallClockTime = DateTime.UtcNow - start;
		}
	}

	class Test
	{
		public string Filename { get; }

		public string OFlag { get; }

		public int ExpectedExitCode { get; }

		public int Timeout { get; }

		public string StandardOutputFilename { get; private set; }

		public StreamWriter StandardOutput { get; private set; }

		public string StandardErrorFilename { get; private set; }

		public StreamWriter StandardError { get; private set; }

		public Test (string filename)
		{
			Filename = filename;
		}

		public Status Run (string runtime, TextWriter output, string output_width)
		{
			output.Write (String.Format ("{{0,-{0}}} ", output_width), Filename);

			using (Process p = new Process ()) {
				p.StartInfo = new ProcessStartInfo (runtime) {
					Arguments = OFlag == null ? Filename : string.Format ("-O={0} {1}", OFlag, Filename),
					UseShellExecute = false,
					RedirectStandardOutput = true,
					RedirectStandardError = true,
				};

				p.StartInfo.EnvironmentVariables [ENV_TIMEOUT] = Timeout.ToString();

				string log_prefix = "";
				if (OFlag != null)
					log_prefix = "." + OFlag.Replace ("-", "no").Replace (",", "_");

				StandardOutputFilename = Filename + log_prefix + ".stdout";
				StandardOutput = new StreamWriter (new FileStream (StandardOutputFilename, FileMode.Create));

				StandardErrorFilename = Filename + log_prefix + ".stderr";
				StandardError = new StreamWriter (new FileStream (StandardErrorFilename, FileMode.Create));

				p.OutputDataReceived += delegate (object sender, DataReceivedEventArgs e) {
					if (e.Data != null) {
						StandardOutput.WriteLine (e.Data);
					} else {
						StandardOutput.Flush ();
						StandardOutput.Close ();
					}
				};

				p.ErrorDataReceived += delegate (object sender, DataReceivedEventArgs e) {
					if (e.Data != null) {
						StandardError.WriteLine (e.Data);
					} else {
						StandardError.Flush ();
						StandardError.Close ();
					}
				};

				var start = DateTime.UtcNow;

				p.Start ();

				p.BeginOutputReadLine ();
				p.BeginErrorReadLine ();

				if (!p.WaitForExit (Timeout * 1000)) {
					// Force the process to print a thread dump
					try {
						Syscall.kill (p.Id, Signum.SIGQUIT);
						Thread.Sleep (1000);
					} catch {
					}

					try {
						p.Kill ();
					} catch {
					}

					output.Write ("timed out");
					return Status.TimedOut;
				} else if (p.ExitCode != ExpectedExitCode) {
					output.Write ("failed, time: {0}, exit code: {1}", (DateTime.UtcNow - start).ToString (TEST_TIME_FORMAT), p.ExitCode);
					return Status.Failed;
				} else {
					output.Write ("passed, time: {0}", (DateTime.UtcNow - start).ToString (TEST_TIME_FORMAT));
					return Status.Passed;
				}
			}
		}

		enum Status
		{
			Passed,
			Failed,
			TimedOut,
		}
	}
}

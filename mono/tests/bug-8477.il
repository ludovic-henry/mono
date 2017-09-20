// This test is meant to make sure Mono doesn't fail when invalid COM invocations are present but never executed.
// See https://bugzilla.xamarin.com/show_bug.cgi?id=8477 for details.

// Metadata version: v4.0.30319
.assembly extern mscorlib
{
  .ver 0:0:0:0
}

.assembly 'bug-8477'
{
  .custom instance void [mscorlib]System.Runtime.CompilerServices.RuntimeCompatibilityAttribute::.ctor() = ( 01 00 01 00 54 02 16 57 72 61 70 4E 6F 6E 45 78   // ....T..WrapNonEx
                                                                                                             63 65 70 74 69 6F 6E 54 68 72 6F 77 73 01 )       // ceptionThrows.
  .hash algorithm 0x00008004
  .ver 0:0:0:0
}

.imagebase 0x00400000
.file alignment 0x00000200
.stackreserve 0x00100000
.subsystem 0x0003       // WINDOWS_CUI
.corflags 0x00000001    //  ILONLY

// =============== CLASS MEMBERS DECLARATION ===================
.class public auto ansi import C
       extends [mscorlib]System.Object
{
  .method public hidebysig newslot specialname virtual 
          instance object  get_Value([opt] object a) runtime managed preservesig internalcall
  {
  } // end of method C::get_Value

  .method public hidebysig newslot specialname virtual 
          instance void  set_Value2([opt] object a,
                                    [opt] object b) runtime managed preservesig internalcall
  {
  } // end of method C::set_Value2

  .property object Value(object)
  {
    .get instance object C::get_Value(object)
  } // end of property C::Value
  .property object Value2(object)
  {
    .set instance void C::set_Value2(object,
                                     object)
  } // end of property C::Value2
} // end of class C


.class private auto ansi beforefieldinit X
       extends [mscorlib]System.Object
{
  .method private hidebysig instance void 
          Test_PropertyOptionalParameters(class C c) cil managed
  {
    // Code size       46 (0x2e)
    .maxstack  8
    IL_0000:  nop
    IL_0001:  ldarg.1
    IL_0002:  brtrue     IL_000c

    IL_0007:  br         IL_002d

    IL_000c:  ldarg.1
    IL_000d:  ldsfld     class [mscorlib]System.Reflection.Missing [mscorlib]System.Reflection.Missing::Value
    IL_0012:  callvirt   instance object C::get_Value(object)
    IL_0017:  call       void [mscorlib]System.Console::WriteLine(object)
    IL_001c:  ldarg.1
    IL_001d:  ldsfld     class [mscorlib]System.Reflection.Missing [mscorlib]System.Reflection.Missing::Value
    IL_0022:  ldc.i4.1
    IL_0023:  box        [mscorlib]System.Int32
    IL_0028:  callvirt   instance void C::set_Value2(object,
                                                                     object)
    IL_002d:  ret
  } // end of method X::Test_PropertyOptionalParameters

  .method private hidebysig static int32 
          Main() cil managed
  {
    .entrypoint
    // Code size       23 (0x17)
    .maxstack  2
    .locals init (class X V_0,
             int32 V_1)
    IL_0000:  nop
    IL_0001:  newobj     instance void X::.ctor()
    IL_0006:  stloc.0
    IL_0007:  ldloc.0
    IL_0008:  ldnull
    IL_0009:  callvirt   instance void X::Test_PropertyOptionalParameters(class C)
    IL_000e:  ldc.i4.0
    IL_000f:  stloc.1
    IL_0010:  br         IL_0015

    IL_0015:  ldloc.1
    IL_0016:  ret
  } // end of method X::Main

  .method public hidebysig specialname rtspecialname 
          instance void  .ctor() cil managed
  {
    // Code size       7 (0x7)
    .maxstack  8
    IL_0000:  ldarg.0
    IL_0001:  call       instance void [mscorlib]System.Object::.ctor()
    IL_0006:  ret
  } // end of method X::.ctor

} // end of class X


// =============================================================

// *********** DISASSEMBLY COMPLETE ***********************

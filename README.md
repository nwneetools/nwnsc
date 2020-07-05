![C/C++ CI](https://github.com/nwneetools/nwnsc/workflows/C/C++%20CI/badge.svg)

# Readme contains outdated information. Update in progress.

nwnsc script compiler

Binaries Avaialble - https://neverwintervault.org/project/nwn1/other/tool/nwnsc-nwn-enhanced-edition-script-compiler

Docker image: glorwinger/nwnsc
To compile all nss in current directory with a container, run

```
docker run --rm -it -v "$(pwd):/tmp" jakkn/nwnsc
```

Further runtime arguments to nwnsc can be given at the end, including targets.

---

This source release includes libraries to read and manipulate various
NWN-related data formats.


The project is laid out into several different components:


External provides various external components not shipped with the WDK.

zlib is the standard zlib compression library (unmodified).

minizip is the zlib/contrib minizip library, with some modifications to improve
performance when doing sequential scan enumeration of large archives.

SkywingUtils includes a smart-pointer class similar in nature to boost's
std::tr1::shared_ptr<>.

Granny2Lib encapsulates tazpn (theo)'s Granny2-reading logic in a single
library, isolated from the rest of the projects.

Gr2Conv is an x86-only executable that can be used to decompress *.gr2 files
out of process.  It is used to support Granny2 reading on non-x86 platforms.

NWNBaseLib provides various common definitions used by the other NWN-related
project code (such as the definition of NWN::OBJECTID).

NWN2MathLib is a math library that encapsupates various useful mathematical
constructs, assembled from various sources.

NWN2DataLib encapsulates logic to process a variety of NWN2-related data
formats other than *.gr2 files (which are handled by Granny2Lib).  For example,
logic to handle MDB models and TRX terrain data files is provided in
NWN2DataLib.  Additionally, the library provides a unified resource accessor
interface (compatible with the BioWare resource manager) to load data files in
the same search order as the game.

NWNScriptLib encapsulates a virtual machine for the compiled NWScript
instruction set (*.ncs files).  The virtual machine can execute *.ncs files
programmatically.  It is up to the user of the script library to provide
implementations of engine action handlers and engine structures.

ModelRenderer is a simple GDI rendering application that utilizes the above
libraries to render a model.  Model handling is simplified and does not support
multiple skeletons per model as per the full rendering logic in the Client
Extension.

NWNScriptConsole is a simple console application that can run NWScript programs
on the command line.  It is a simple frontend host for NWNScriptLib.  Only a
small portion of the NWN2 nwscript.nss is implemented by NWNScriptConsole.

NWNScriptCompiler is a compiler driver (shell) for NWScript compilation, based
on NWNScriptCompilerLib.

NWNScriptCompilerDll is a DLL version of the NWScript source compiler, used by
the compiler toolset plugin.

NWNScriptCompilerLib is a static library for compiling NWScript source text to
compiled NWScript (*.ncs).

NWN2ToolsetCompilerPlugin is a .NET 2.0 C# DLL that acts as a toolset plugin to
replace the standard script compiler with the version exported by
NWNScriptCompilerDll.

NWNScriptJIT is a just-in-time (JIT) compilation system for *.ncs files
(NWScript compiled scripts).  The NWNScriptJIT module can be used to execute
NWScript scripts at high speed.  It requires the .NET Framework and must be
built from within Visual Studio.

NWNScriptJITIntrinsics is a support module for NWNScriptJIT.dll.  Generated
.NET assemblies for JIT'd scripts depend on this module.  This module requires
the .NET Framework and must be built from within Visual Studio.

ListModuleAreas is a sample program that shows how to use the included
libraries to traverse the contents of area files in a module.

ListModuleModels is a sample program that shows how to use the included
libraries to enumerate all model files accessible from within a module.

UpdateModTemplates is a sample program that shows how to use the included
libraries to read data, modify, and write back data in placed object instances
in a module's area files.


All of the libraries are intended to be built as static libraries (*.lib).  The
libraries generally expect the caller to provide any thread synchronization if
used in a multithreaded environment.



A good place to start in understanding how to use the libraries would be to
examine how the ModelRenderer program loads and processes a .mdb model.  For
the ModelRenderer program to work, you will need NWN2 installed on your
computer.  You need to tell the ModelRender program where your NWN2 install is
and what module's data files should be loaded by setting environment variables
before executing it; see WinMain in ModelRenderer\ModelRenderer.cpp for more
details.

Skeleton processing requires an NWN2 install with granny2.dll available and is
thus only available on x86 builds.

Most users of the library set will want to link to SkywingUtils, NWNBaseLib,
NWN2MathLib, NWN2DataLib, zlib, and minizip.  It is typical for a user to
create a single ResourceManager object (from NWN2DataLib) and use it to load
data files in from disk.  This strategy makes the most out of the caching and
load-time optimization logic located in the ResourceManager.

The Client Extension uses the included libraries to support it's internal
handling of NWN2 data files.


Data file handling for specific formats:

.TRX / .TR? and .MDB files are handled by class TrxFileReader.
MDB SKIN meshes are handled by class SkinMesh.
MDB RIGD meshes are handled by class RigidMesh.
MDB HAIR points are handled by class HairPoint.
MDB HELM points are handled by class HelmPoint.
MDB WALK meshes are handled by class WalkMesh.
MDB COL2/COL3 meshes are handled by class CollisionMesh.
MDB COLS spheres are handled by class CollisionSphere (not fully implemented).
TRX ASWM records are handled by class AreaSurfaceMesh.
TRX WATR records are handled by class AreaWaterMesh.
TRX TRRN records are handled by class AreaTerrainMesh.
GR2 Skeleton structures are parsed by Granny2Lib and wrapped by ModelSkeleton.
2DA files are handled by class TwoDAFileReader.
GFF files are handled by class GffFileReader and class GffFileWriter.
NCS files are handled by class NWScriptReader.
NSS files are handled by class NscCompiler.


Model system design notes:
--------------------------

The model system is designed to work with shared model meshes.  A logical model
is represented by class ModelCollider.  Each ModelCollider owns its collision
meshes (in world space) and maintains shared references to its meshes in the
form of a pointer to a ModelInstance object.  ModelInstance-based meshes are
stored in model local space.

It is intended for a single ModelCollider to represent a model part (i.e. a
chest model), and for a single ModelSkeleton to represent a skeleton (i.e. a
cloak skeleton).  These model components would typically be aggregated into a
single overarching "whole model".  A greatly simplified example of this can be
seen in the ModelRenderer's WorldObject class.

The stock game and the Client Extension's model system support model attachment
and multiple skeletons, but for clarity, these features are omitted from the
base model system in the ModelRender program (in the interest of cutting down
down on extra layers of abstraction).


GFF file writer design notes:
-----------------------------

There are two build-time configuration options for the GFF writing system.
These options can be set in NWN2DataLib\GffFileWriter.h.  They only impact the
run-time performance (not the final output), by tuning the GFF writer for two
different scenarios.


Define GFFFILEWRITER_PRETRACK_STRUCTS to zero to optimize the writing system
for in-place editing of large data trees that are written out only once per
GffFileWriter instance.  This mode of operation is slower if you save the same
GffFileWriter object and repeatedly commit it, because the data tree is
reindexed on the fly for each write commit operation.

Define GFFFILEWRITER_PRETRACK_STRUCTS to one to optimize the writing system for
for editing of large data trees that are repeatedly written out with the same
GffFileWriter instance.  This mode of operation imposes a significant
performance penalty if your code frequently deletes large sections of a GFF
data tree, as the pre-tracked struct index must be updated recursively for
deletions of struct types.


Most users will find defining GFFFILEWRITER_PRETRACK_STRUCTS to zero to be the
best performing option.  This is the default that the library ships configured
with.


NWNScriptLib design notes:
--------------------------

The NWScriptVM object encapsulates a complete, compiled NWScript execution
environment.  In order to use the NWScriptVM, it is important to understnad
several design principles.

The script VM is single threaded, but reentrant.  (That is, multiple recursive
entries into the script VM are permitted.)  Each script called by the script VM
takes the form of a call to a script program (represents by a NWScriptReader
object).

Script execution nominally begins with a call to NWScriptVM::ExecuteScript,
which transfers control to the entry point symbol of the script program that
has been provided.  Normally, an entry point has the following prototype in
NWScript:

void main();
int StartingConditional();

The script VM supports both 'main' and 'StartingConditional' prototypes.  The
latter may return an integer value, whereas the former has no return value,
although the script VM will return zero to the caller if the script was of
the 'main' sort.  Generally, a programmatic call to a script already knows
a-priori whether it is a 'StartingConditional' (i.e. whether it returns a valid
return value).

Additionally, the script VM supports the NWN2-specific extension of NWScript
that allows parameterized script entry point symbols (i.e. entry point symbols
that are passed parameters by the programmatic caller of the core
NWScriptVM::ExecuteScript API).

Script parameters are always passed to ExecuteScript as string values, but they
will internally be dynamically converted to the appropriate type when
referenced by the script itself.

The following formatting conventions must be observed for script parameters
that are passed in to ExecuteScript:

- Strings are passed as-is.
- Integers are passed as base 10 signed integral values (i.e. formatted as %d).
- Floats are passed as base 10 floating point values (i.e. formatted as %g).
- Object IDs are passed as base 10 signed interval values (i.e. formatted as
  %d).

Engine structures cannot be passed as script parameters in the current version
of the script VM.

Unlike the default script VM in NWN2, the NWNScriptLib does not require the
user of a the script VM to compile parameterized scripts on each exeuction in
order to determine the types of arguments.  Dynamic typing is implemented for
script entry point parameters in order to eliminate this requirement.

An attempt to call a script entry point with a mismatched number of parameters
will result in an invalid type error within the script VM and will result in
the script call chain being terminated (with control returned to the user).


While the script execution environment already supports the intrinsic
mathematical operators supported by the compiled NWScript instruction set, most
useful work involves exposing customized functionality to the scripting
environment.

Custom functionality may be exposed through two different extension points,
both of which are fully supported by the script VM:

- A user of the script VM may expose functions to the script environment by
  implementing a series of "action handlers".  An action is a function that has
  been prototyped in the nwscript.nss file, and it is implemented in native
  code by the user of the script VM.  The user's action handler dispatcher is
  invoked (INWScriptActions::OnExecuteAction) when the script makes a call to
  any script action.

  It is up to the user of the NWNScriptLib to implement any and all desired
  script action handlers.  The first function prototype in nwscript.nss
  corresponds to action ordinal 0, the second correspond to action ordinal 1,
  and so on and soforth.

  The script VM itself is agnostic to the specifics of each action's
  implementation or prototype, as is the script compiler.  (It is possible to
  create a completely new action set for use in a custom program using a custom
  written nwscript.nss file with the script compiler, plus a custom written
  INWScriptActions interface implementation in conjuction with the NWScriptVM.)

  Action handlers are required to adhere to the following rules for argument
  and return value passing:

  1. All arguments are passed on the stack, with the first argument being the
     first parameter, and so on and so forth.  The action handler must remove
     all arguments from the stack before executing a return sequence.

     The exception to this rule involves script situation arguments, which are
     typed as "action" in the NWScript compiler.  These arguments are not
     passed on the stack, and there may be at most one per action handler call.

     The rules for action handlers are described in the documentation for
     "script situations", located below.

  2. If an action handler returns a value to the script environment, it must
     push the return value onto the script stack after all arguments have been
     removed from the stack.  Note that adding or removing a return value thus
     changes the fundamental calling convention of a particular action handler
     from the script compiler's perspective.

  Action handlers may access the script stack for these purposes by calling
  the NWScriptStack::StackPush* and NWScriptStack::StackPop* APIs with the
  NWScriptStack instance passed in to INWScriptActions::OnExecuteAction.


- Additionally, a user of the script VM may expose "engine structures" to the
  script environment.  An "engine structure" is simply a reference to an opaque
  structure that is meaningful only to engine action handlers (although the
  script environment may copy references, compare engine structures with the
  aid of EngineStructure::CompareEngineStructure, as well as pass engine
  structure references to action handlers).

  An engine structure is useful to expose a data item that the script
  environment may usefully store and then use later with a future call to an
  action handler.

  The INWScriptActions::CreateEngineStructure API, provided by the user of the
  script VM, creates a default-initialized engine structure of the given engine
  structure type.

  The script VM is entirely agnostic to the implementation of a particular type
  of engine structure.  There is support for up to 9 engine structure types to
  be defined and exposed to the script environment.


The script VM also supports the concept of a "script situation".  A script
situation, referred to as the "action" type in the NWScript compiler, is a copy
of the script VM's internal state that is snapshotted when a call to an engine
action handler that takes an "action"-typed parameter is made.

The script situation can be used to call back to a specific section of script
code at a later time, even after the script VM has returned control back to the
user.  Script situations are useful for packaging deferred calls to a
particular section of script code.  As an example, the NWN2 implementation of
NWScript has a "DelayCommand" engine action handler, which allows a section of
script code to be queued for later execution.

Script situations are not passed as standard arguments to action handlers.
Instead, the action handler must know a-priori that it expects to receive a
script situation (action) argument type.  Then, the action handler may retrieve
a reference to the last saved script situation with a call to the
NWScriptVM::GetSavedState API.

It is important to make a copy of the saved state object before control returns
or it may be overwritten later.  The saved state object maintains a copy of all
relevant data on the virtual machine stack (including references to the
script program code and any engine structures that were active).  By default,
if the saved state object is not copied, it will be deleted when the outermost
call to NWScriptVM::ExecuteScript returns control to the user.

The script VM allows a script situation to be activated later, effectively
calling back into the "middle" of a script.  This operation is performed by
invoking the NWScriptVM::ExecuteScriptSituation API.


Two manifest object id values may be associated with a particular execution of
a script entry point symbol:

- The invalid object id represents no object.  It is recommended that the value
  0x7F000000 be used for this parameter for compatibility purposes.  The script
  language refers to this constant by name of OBJECT_INVALID.

- The self object id represents the object whose context a script call is being
  executed in.  The script language refers to this constant by name of
  OBJECT_SELF.

The values of these constants is set by the user on each call to ExecuteScript.


A NWScript action handler may abort a recursive script by calling the
NWScriptVM::AbortScript function.  This terminates the entire script call chain
once control returns to the NWScriptVM.


If any script in a script call chain encounters an error, such as an illegal
stack reference, the script call chain is aborted as though AbortScript were
called by the user.


Verbose debug output is made to the user with the aid of the
NWScriptVM::SetDebugLevel API.  Debug output is routed through the
IDebugTextOut interface implementation that is provided by the user.

A static analysis system for NWScript is available in the NWScriptAnalyzer
component of the NWNScriptLib.  This class allows the compiled bytecode of a
NWScript program to be examined in order to glean more information about the
script program in a programmatic, static (offline) fashion.


---------------------------------------

CREDITS

Copyright (C) 2008-2011 Ken Johnson (Skywing).
Portions of NWN2DataLib's TRX handling are based on information from Tero
Kivinen.
Major portions of NWNScriptCompilerLib were written by Edward T. Smith
(Torlack).
Portions of NWNScriptCompilerLib were contributed by OpenKnights.



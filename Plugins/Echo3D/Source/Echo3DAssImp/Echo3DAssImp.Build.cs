// Copyright Epic Games, Inc. All Rights Reserved.
using System;
using System.IO;
using System.Collections.Generic;
using UnrealBuildTool;

public class Echo3DAssImp : ModuleRules
{
	//How to build AssImp:
	//Grab from github, open in CMAKE
	//Starting with Use default options
	// enable ASSIMP_BUILD_DRACO
	// open project in VS
	// I recommend building as RelWithDebInfo (release with debug info) for debugging sanity, but Release and MinSizeRel make sense too
	// copy dll,lib,pdb,and both includes to plugins folder under assimp.
	// there are two includes - one generated which has config.h and the rest in the repo source folder.
	// draco and some of the libs are not all in the places you would expect.
	// you want assimp-vc142-mt.dll, assimp-vc142-mt.pdb, assimp-vc142-mt.lib, draco.dll, draco.lib, draco.pdb. add "d" before the dot if making a DEBUG build of them (and set the correct flag below - untested)

	public class MyArtifact
	{
		public string dllName;
		public string libName;
		public string pdbName;

		public MyArtifact(string setDllName, string setLibName, string setPdbName)
		{
			dllName = setDllName;
			libName = setLibName;
			pdbName = setPdbName;
		}

		public static MyArtifact CreateArtifact(string baseName)
		{
			return new MyArtifact(
				baseName + ".dll",
				baseName + ".lib",
				baseName + ".pdb"
			);
		}
	}

	public Echo3DAssImp(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		//////////////////////////////////////
		//BUILD FLAGS
			//Should we build against the normal or debug versions (debug versions have a "d" letter before the dot
				const bool includeDebugDLL = false; //DEFAULT
				//const bool includeDebugDLL = true;//might not work?
		
			//copy the PDB file so can step into assimp source at runtime for debugging - most people will not want/need this
				const bool includePDB = false; //DEFAULT
				//const bool includePDB = true;//copy PDB file over as well. probably won't work if you don't have the PDB file, but you can build AssImp RelWithDebInfo and generate your own artifacts(note: definitely will need to replace dll and lib files if you do so!)

			//tries to copy draco over as well. use if built AssImp with ASSIMP_BUILD_DRACO
				const bool includeDraco = true; //DEFAULT
				//const bool includeDraco = false; //turn off draco
		/////////////////////////////////////
		
		string assImpDir = Path.GetFullPath(Path.Combine(PluginDirectory, "Source", "ThirdParty", "assimp"));
		string assImpIncludeDir = Path.Combine(assImpDir, "include");
		string assImpLibDir = Path.Combine(assImpDir, "lib");
		string assImpDLLDir = Path.Combine(assImpDir, "bin");
		
		List<MyArtifact> artifacts = new List<MyArtifact>();
		
		string debugSuffix = "";
		if (includeDebugDLL)
		{
			debugSuffix = "d";
		}
		
		string assImpBase = "assimp-vc142-mt" + debugSuffix;
		string dracoBase = "draco" + debugSuffix;

		artifacts.Add(MyArtifact.CreateArtifact(assImpBase));
		if (includeDraco)
		{
			artifacts.Add(MyArtifact.CreateArtifact(dracoBase));
		}
		/*
		//debug dirs
		Console.WriteLine("assImpDir = "+assImpDir);
		Console.WriteLine("\t"+"Includes = "+assImpIncludeDir);
		Console.WriteLine("\t"+"Libs = "+assImpLibDir);
		Console.WriteLine("\t"+"Dlls = "+assImpDLLDir);
		*/
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...		
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
				assImpIncludeDir,
			}
		);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
		);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
				"ProceduralMeshComponent",
				"ImageWrapper",
			}
		);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
		
		// https://docs.unrealengine.com/4.27/en-US/ProductionPipelines/BuildTools/UnrealBuildTool/ThirdPartyLibraries/	
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			//probably better to crash and burn during load for now
			//NOTE 2: if you disable this, you need to put the dll in the project binaries folder ...sigh...
			//BAD//PublicDelayLoadDLLs.Add("assimp-vc142-mt.dll");
			foreach(MyArtifact artifact in artifacts)
			{
				string dllName = artifact.dllName;
				string pdbName = artifact.pdbName;
				string libName = artifact.libName;

				string dllFile = Path.Combine(assImpDLLDir, dllName);
				string pdbFile = Path.Combine(assImpDLLDir, pdbName);
				string libFile = Path.Combine(assImpLibDir, libName);
				
				/*
				const int UseBinDir = 0; //want this one?
				const int UseTargDir = 2;
				const int UseDefault = 1;
				//int whichBuildCase = UseDefault; 
				int whichBuildCase = UseBinDir; 
				switch(whichBuildCase)
				{
					case UseDefault:
						RuntimeDependencies.Add(dllFile, StagedFileType.NonUFS);
						break;
					case UseBinDir:
				*/
						RuntimeDependencies.Add("$(BinaryOutputDir)/"+dllName, dllFile, StagedFileType.NonUFS); //outputs to binaries folder?
						if (includePDB)
						{
							RuntimeDependencies.Add("$(BinaryOutputDir)/"+pdbName, pdbFile, StagedFileType.NonUFS); //outputs to binaries folder?
						}
				/*
						break;

					case UseTargDir:
						RuntimeDependencies.Add("$(TargetOutputDir)/"+dllName, dllFile, StagedFileType.NonUFS); //outputs to binaries folder?
						if (includePDB)
						{
							RuntimeDependencies.Add("$(TargetOutputDir)/"+pdbName, pdbFile, StagedFileType.NonUFS); //outputs to binaries folder?
						}
						break;
					default:
						throw new System.Exception("Case not implemented: "+whichBuildCase);
				}//end switch,
				*/
				
				PublicAdditionalLibraries.Add(libFile);
			}
			
			PublicAdditionalLibraries.AddRange(
				new string[]
				{
					//libFile, //MOVED ABOVE
					//Path.Combine(assImpLibDir, "IrrXML.lib"),
					//Path.Combine(assImpLibDir, "zlib.lib"),
					//Path.Combine(assImpLibDir, "zlibstatic.lib"),
					//Path.Combine(assImpLibDir, "libassimp.so"),
				}
			);
		}
		else
		{
			//not in spec and don't have a test machine
			throw new Exception("Only Win64 supported!");

			//NEVER TESTED
			/*
			//~approx from runtimemeshloader build script:
			if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				PublicDelayLoadDLLs.Add("assimp-vc142-mt.dll");
				//RuntimeDependencies.Add("$(TargetOutputDir)/Foo.dll", Path.Combine(PluginDirectory, "Source/ThirdParty/bin/Foo.dll"));
				RuntimeDependencies.Add("$(TargetOutputDir)/Foo.dll", Path.Combine(assImpDLLDir, "assimp-vc142-mt.dll"));
				PublicAdditionalLibraries.AddRange(
					new string[]
					{
						Path.Combine(assImpLibDir, "libassimp.so"),
					}
				);
			}
			*/
		}
	}
}

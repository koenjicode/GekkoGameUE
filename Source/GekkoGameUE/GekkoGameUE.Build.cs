// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GekkoGameUE : ModuleRules
{
	public GekkoGameUE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UnrealEd",
			"GekkoNetUE",
			"GekkoNet",
			"Redo",
		});
		
		PrivateDependencyModuleNames.AddRange(new string[] {  });
		PrivateDependencyModuleNames.Add("OnlineSubsystem");
	}
}

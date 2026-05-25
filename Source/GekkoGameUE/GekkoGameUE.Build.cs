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
			"GekkoNetUE",
			"GekkoNet",
			"Redo",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"CoreOnline"
		});

		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd"
				}
			);
		}
		
		PrivateDependencyModuleNames.AddRange(new string[] {  });
		PrivateDependencyModuleNames.Add("OnlineSubsystem");
	}
}

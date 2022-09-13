// Fill out your copyright notice in the Description page of Project Settings.


#include "EchoBlueprintUtil.h"


FString UEchoBlueprintUtil::ResolveQueryDebugInfo(const FEchoQueryDebugInfo &queryDebugState)
{
	return EchoHelperLib::ResolveQueryDebugInfo(queryDebugState);
}
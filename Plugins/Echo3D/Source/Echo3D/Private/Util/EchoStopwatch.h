#pragma once

#include "CoreMinimal.h"
#include "Containers/UnrealString.h"

struct FEchoStopwatch
{
	//typedef float TimeType;
	typedef double TimeType;

	TimeType accumulator;
	TimeType startTime;
	bool bRunning;

	FEchoStopwatch()
	: accumulator(0), startTime(0), bRunning( false )
	{
	}
	TimeType GetElapsedSeconds() const
	{
		TimeType ret = accumulator;
		if (bRunning)
		{
			TimeType now = ReadCurrentTime();
			ret += (now - startTime);
		}
		return ret;
	}

	TimeType GetElapsedMilliseconds() const
	{
		return 1000.0f * GetElapsedSeconds();
	}

	void Start()
	{
		//TODO: maybe use a start/stop counter for reentrant behavior?
		if (bRunning)
		{
			UE_LOG(LogTemp, Error, TEXT("Stopwatch::Start - already running!"));
			return;
		}
		bRunning = true;
		startTime = ReadCurrentTime();
	}

	void Stop()
	{
		if (!bRunning)
		{
			UE_LOG(LogTemp, Error, TEXT("Stopwatch::Stop - not running!"));
			return;
		}
		bRunning = false;
		TimeType now = ReadCurrentTime();
		accumulator += (now - startTime);
		//StartTime = now;
	}

	void Reset()
	{
		bRunning = false;
		startTime = 0;
		accumulator = 0;
	}

	void PrintTimer(const FString &label)
	{
		//UE_LOG(LogTemp, Error, TEXT("Timer [%s]: %f"), *label, (float)GetElapsedSeconds());
		UE_LOG(LogTemp, Error, TEXT("Timer [%s]: %lf"), *label, (float)GetElapsedSeconds());
	}

	static TimeType ReadCurrentTime();
};

//Usage FEchoAutoStopwatch(
struct FEchoAutoStopwatch
{
	const FString *label;
	FEchoStopwatch timer;

	//NOTE: this is only meant to have automatic scope. use outside of that could crash if the FSTring isn't persistent!
	FEchoAutoStopwatch(const FString &setLabel)
	{
		label = &setLabel;
		timer.Start();
	}

	~FEchoAutoStopwatch()
	{
		timer.Stop();
		const FString *_label = (label != nullptr) ? label : &EchoStringConstants::EmptyString;
		timer.PrintTimer(*_label );
	}

};
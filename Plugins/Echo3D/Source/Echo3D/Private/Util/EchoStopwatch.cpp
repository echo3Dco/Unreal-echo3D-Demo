#include "EchoStopwatch.h"
#include "../EchoHelperLib.h"
//#include "Unix/UnixPlatformTime.h"
//#include "GenericPlatform/GenericPlatformTime.h"
//#include "HAL/PlatformTime.h"

FEchoStopwatch::TimeType FEchoStopwatch::ReadCurrentTime()
{
	//return EchoHelperLib::GetCurrentTime();
	return FPlatformTime::Seconds();
	//return FGenericPlatformTime::Seconds();
}
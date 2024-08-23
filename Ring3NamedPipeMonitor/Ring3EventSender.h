#ifndef RING3_EVENT_SENDER
#define RING3_EVENT_SENDER
#include <Windows.h>
#include <vector>

BOOL SendEvent(const std::vector<std::pair<PVOID, ULONG>>& datas);


#endif

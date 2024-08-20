#pragma once
#include <ntddk.h>

#define DBG

#ifndef DBG
#define DBG(Message, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[PTE-Viewer] " __FUNCTION__ "() - " Message "\n", __VA_ARGS__)
#endif
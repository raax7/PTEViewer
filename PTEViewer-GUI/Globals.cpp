#include "Globals.h"

#include "Driver.h"
#include "PollManager.h"
#include "ProcessManager.h"
#include "ProcessListUI.h"

#include "PTViewerUI.h"

#ifdef _DEBUG 
#define DEBUG_CHECK(Global) \
if (Global == nullptr) \
{ \
__debugbreak(); \
}
#else
#define DEBUG_CHECK(Global) ((void)0)
#endif

Driver* g_Driver = nullptr;
PollManager* g_PollManager = nullptr;
ProcessManager* g_ProcessManager = nullptr;
ProcessListUI* g_ProcessListUI = nullptr;

std::vector<std::unique_ptr<PTViewerUI>> Globals::g_InspectedPTs;

void Globals::Setup()
{
    g_Driver = new Driver;
    g_PollManager = new PollManager;
    g_ProcessManager = new ProcessManager;
    g_ProcessListUI = new ProcessListUI;
}

Driver* Globals::GetDriver()
{
    DEBUG_CHECK(g_Driver);
    return g_Driver;
}
PollManager* Globals::GetPollManager()
{
    DEBUG_CHECK(g_PollManager);
    return g_PollManager;
}
ProcessManager* Globals::GetProcessManager()
{
    DEBUG_CHECK(g_ProcessManager);
    return g_ProcessManager;
}
ProcessListUI* Globals::GetProcessListUI()
{
    DEBUG_CHECK(g_ProcessListUI);
    return g_ProcessListUI;
}

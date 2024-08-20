#pragma once
#include <memory>
#include <vector>

class Driver;
class PollManager;
class ProcessManager;
class ProcessListUI;

class PTViewerUI;

namespace Globals
{
    void Setup();

    Driver* GetDriver();
    PollManager* GetPollManager();
    ProcessManager* GetProcessManager();
    ProcessListUI* GetProcessListUI();

    extern std::vector<std::unique_ptr<PTViewerUI>> g_InspectedPTs;
}

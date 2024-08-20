#pragma once
#include <vector>
#include <memory>
#include "PollEntry.h"

class PollManager
{
public:
    PollManager() = default;
    ~PollManager() = default;

public:
    void AddPollEntry(std::shared_ptr<PollEntry> entry);
    void RemovePollEntry(std::shared_ptr<PollEntry> entry);

    void PollAll();
    void ForcePollAll();

private:
    std::vector<std::shared_ptr<PollEntry>> m_PollEntries;
};

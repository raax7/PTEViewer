#include "PollManager.h"

void PollManager::AddPollEntry(std::shared_ptr<PollEntry> entry)
{
    m_PollEntries.push_back(entry);
}
void PollManager::RemovePollEntry(std::shared_ptr<PollEntry> entry)
{
    m_PollEntries.erase(std::remove(m_PollEntries.begin(), m_PollEntries.end(), entry), m_PollEntries.end());
}

void PollManager::PollAll()
{
    for (const auto& Entry : m_PollEntries)
    {
        Entry->Poll();
    }
}
void PollManager::ForcePollAll()
{
    for (const auto& Entry : m_PollEntries)
    {
        Entry->ForcePoll();
    }
}

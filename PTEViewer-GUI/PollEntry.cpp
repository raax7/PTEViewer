#include "PollEntry.h"

void PollEntry::Poll()
{
    auto Now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(Now - m_LastPolled).count() >= IntervalMs)
    {
        m_PollCallback();
        m_LastPolled = Now;
        m_PollOccurred = true;
        m_PollCount++;
    }
}
void PollEntry::ForcePoll()
{
    m_PollCallback();
    m_LastPolled = std::chrono::steady_clock::now();
    m_PollOccurred = true;
    m_PollCount++;
}

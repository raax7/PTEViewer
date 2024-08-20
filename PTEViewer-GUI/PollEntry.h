#pragma once
#include <functional>
#include <chrono>

class PollEntry
{
public:
    using Callback = std::function<void(void)>;

    PollEntry(Callback PollCallback, int& IntervalMs)
        : IntervalMs(IntervalMs), m_PollOccurred(false), m_PollCount(0), m_PollCallback(PollCallback), m_LastPolled(std::chrono::steady_clock::time_point())
    {
    }

public:
    bool PollOccurred() { return m_PollOccurred; m_PollOccurred = false; }
    int PollCount() { return m_PollCount; }

public:
    void Poll();
    void ForcePoll();

public:
    // This member is public to facilitate ease of use with ImGui's SliderInt.
    int& IntervalMs;

private:
    bool m_PollOccurred;
    int m_PollCount;

    Callback m_PollCallback;
    std::chrono::time_point<std::chrono::steady_clock> m_LastPolled;
};

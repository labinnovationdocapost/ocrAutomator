#pragma once

class SlaveState
{
public:
	SlaveState() {}
	int NbThread = 0;
	std::atomic<int> PendingProcessed{ 0 };
	std::atomic<int> PendingNotProcessed{ 0 };
	std::mutex ClientMutex{};
};
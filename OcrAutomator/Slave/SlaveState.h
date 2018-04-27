#pragma once

class SlaveState
{
public:
	SlaveState() {}
	int NbThread = 0;
	std::atomic<int> PendingProcessed{ 0 };
	std::atomic<int> PendingNotProcessed{ 0 };
	std::atomic<bool> Terminated{ false };
	std::mutex ClientMutex{};
	std::string Name;
	boost::thread* Thread = nullptr;

	~SlaveState()
	{
		delete Thread;
	}
};
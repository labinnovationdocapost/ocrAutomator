#pragma once
#include <mutex>
#include <condition_variable>
#include <thread>
#include <stdio.h>

class AutoResetEvent
{
public:
	explicit AutoResetEvent(bool initial = false);

	void Set();
	void Reset();

	bool WaitOne();

private:
	AutoResetEvent(const AutoResetEvent&);
	AutoResetEvent& operator=(const AutoResetEvent&); // non-copyable
	bool flag_;
	std::mutex protect_;
	std::condition_variable signal_;
};

inline AutoResetEvent::AutoResetEvent(bool initial)
	: flag_(initial)
{
}

inline void AutoResetEvent::Set()
{
	std::lock_guard<std::mutex> _(protect_);
	flag_ = true;
	signal_.notify_all();
}

inline void AutoResetEvent::Reset()
{
	std::lock_guard<std::mutex> _(protect_);
	flag_ = false;
}

inline bool AutoResetEvent::WaitOne()
{
	std::unique_lock<std::mutex> lk(protect_);
	while (!flag_) // prevent spurious wakeups from doing harm
		signal_.wait(lk);
	flag_ = false; // waiting resets the flag
	return true;
}

class Semaphore
{
public:
	explicit Semaphore(int initial = 0);

	void Release();
	void Release(int nb);

	bool Wait(int nb);

private:
	Semaphore(const Semaphore&);
	Semaphore& operator=(const Semaphore&); // non-copyable
	int current;
	std::mutex protect_;
	std::condition_variable signal_;
};

inline Semaphore::Semaphore(int initial)
	: current(initial)
{
}

inline void Semaphore::Release()
{
	Release(1);
}
inline void Semaphore::Release(int nb)
{
	std::lock_guard<std::mutex> _(protect_);
	current += nb;
	signal_.notify_all();
}

inline bool Semaphore::Wait(int nb)
{
	std::unique_lock<std::mutex> lk(protect_);
	while (nb > current) // prevent spurious wakeups from doing harm
		signal_.wait(lk);
	current -= nb; // waiting resets the flag
	return true;
}
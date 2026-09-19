#pragma once

#include <atomic>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>
#include "Ast.h"
#include "Core.h"
#include "Host.h"

namespace TGX::Shell
{
struct Task
{
	int pid = 0;
	String name;
	NodeRef program;
	Vector<String> args;
	Ref<std::atomic<bool>> cancelled;
};

struct ProcessInfo
{
	int pid = 0;
	String name;
	bool running = false;
};

class TaskPool
{
private:
	struct Entry
	{
		String name;
		bool running = false;
		Ref<std::atomic<bool>> cancelled;
	};

	static constexpr std::size_t NUM_WORKERS = 4;

	Host *host = nullptr;
	Vector<std::thread> workers;
	Queue<Task> queue;
	std::map<int, Entry> processes;

	mutable std::mutex mutex;
	std::condition_variable condition;
	std::atomic<bool> running{false};

	void Work();

public:
	~TaskPool();

	void Start(Host *inHost);
	void Stop();
	void Enqueue(Task task);
	bool IsRunning() const;

	Vector<ProcessInfo> List() const;
	bool Kill(int pid);
};
} // namespace TGX::Shell

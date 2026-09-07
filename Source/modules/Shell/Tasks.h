#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "Ast.h"
#include "Core.h"
#include "Host.h"

namespace TGX::Shell
{
struct Task
{
	NodeRef program;
	Vector<String> args;
};

class TaskPool
{
private:
	static constexpr std::size_t NUM_WORKERS = 4;

	Host *host = nullptr;
	Vector<std::thread> workers;
	Queue<Task> queue;

	std::mutex mutex;
	std::condition_variable condition;
	std::atomic<bool> running{false};

	void Work();

public:
	~TaskPool();

	void Start(Host *inHost);
	void Stop();
	void Enqueue(Task task);
	bool IsRunning() const;
};
} // namespace TGX::Shell

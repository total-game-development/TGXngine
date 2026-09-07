#include "Tasks.h"
#include "Interpreter.h"

namespace TGX::Shell
{
TaskPool::~TaskPool()
{
	Stop();
}

void TaskPool::Start(Host *inHost)
{
	if (running.load())
	{
		return;
	}

	host = inHost;
	running.store(true);

	for (std::size_t index = 0; index < NUM_WORKERS; ++index)
	{
		workers.emplace_back([this] { Work(); });
	}
}

void TaskPool::Stop()
{
	if (!running.exchange(false))
	{
		return;
	}

	condition.notify_all();

	for (std::thread &worker : workers)
	{
		if (worker.joinable())
		{
			worker.join();
		}
	}

	workers.clear();

	std::lock_guard<std::mutex> lock(mutex);

	while (!queue.empty())
	{
		queue.pop();
	}
}

void TaskPool::Enqueue(Task task)
{
	if (!running.load())
	{
		return;
	}

	{
		std::lock_guard<std::mutex> lock(mutex);
		queue.push(std::move(task));
	}

	condition.notify_one();
}

bool TaskPool::IsRunning() const
{
	return running.load();
}

void TaskPool::Work()
{
	while (running.load())
	{
		Task task;

		{
			std::unique_lock<std::mutex> lock(mutex);

			condition.wait(lock, [this] { return !queue.empty() || !running.load(); });

			if (!running.load())
			{
				return;
			}

			task = std::move(queue.front());
			queue.pop();
		}

		Interpreter interpreter(host);
		interpreter.Run(task.program, task.args);
	}
}
} // namespace TGX::Shell

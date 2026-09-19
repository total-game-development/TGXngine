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

	{
		std::lock_guard<std::mutex> lock(mutex);

		for (auto &entry : processes)
		{
			entry.second.cancelled->store(true);
		}
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

	processes.clear();
}

void TaskPool::Enqueue(Task task)
{
	if (!running.load())
	{
		return;
	}

	if (!task.cancelled)
	{
		task.cancelled = std::make_shared<std::atomic<bool>>(false);
	}

	{
		std::lock_guard<std::mutex> lock(mutex);

		processes[task.pid] = {task.name, false, task.cancelled};
		queue.push(std::move(task));
	}

	condition.notify_one();
}

bool TaskPool::IsRunning() const
{
	return running.load();
}

Vector<ProcessInfo> TaskPool::List() const
{
	std::lock_guard<std::mutex> lock(mutex);

	Vector<ProcessInfo> list;

	for (const auto &[pid, entry] : processes)
	{
		list.push_back({pid, entry.name, entry.running});
	}

	return list;
}

bool TaskPool::Kill(int pid)
{
	std::lock_guard<std::mutex> lock(mutex);

	const auto found = processes.find(pid);

	if (found == processes.end())
	{
		return false;
	}

	found->second.cancelled->store(true);

	if (!found->second.running)
	{
		processes.erase(found);
	}

	return true;
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

			const auto found = processes.find(task.pid);

			if (found == processes.end() || task.cancelled->load())
			{
				continue;
			}

			found->second.running = true;
		}

		Interpreter interpreter(host);
		interpreter.SetCancel(task.cancelled.get());
		interpreter.Run(task.program, task.args);

		std::lock_guard<std::mutex> lock(mutex);
		processes.erase(task.pid);
	}
}
} // namespace TGX::Shell

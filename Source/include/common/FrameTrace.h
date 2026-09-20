#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include "Core.h"
#include "Logs.h"

namespace TGX
{
// Frame times, so a stutter can be read rather than felt. Every frame's wall
// clock and the work inside it are kept for a few seconds and then reported as
// a spread: an average hides exactly the frames that are the complaint.
class FrameTrace
{
public:
	void Begin()
	{
		frameStart = Clock::now();
	}

	void Work()
	{
		lastWork = Milliseconds(frameStart, Clock::now());

		work.push_back(lastWork);
	}

	void End(float budget)
	{
		const auto now = Clock::now();
		const float spent = Milliseconds(frameStart, now);

		total.push_back(spent);

		counted++;

		// A frame that missed its slot, said as it happens and split where the
		// time went: what the engine did, against what the swap waited for.
		// The second is the driver's or the compositor's, not the engine's.
		if (spent > budget * 2.0f)
		{
			Log::Warning("Frame " + std::to_string(counted) + " took " + Figure(spent) +
						 "ms: " + Figure(lastWork) + "ms of work, " + Figure(spent - lastWork) + "ms in the swap");

			std::fflush(stdout);
		}

		if (reportedAt == Clock::time_point{})
		{
			reportedAt = now;
			return;
		}

		if (std::chrono::duration<float>(now - reportedAt).count() < REPORT_EVERY)
		{
			return;
		}

		Report(budget);

		reportedAt = now;
		total.clear();
		work.clear();
	}

private:
	using Clock = std::chrono::steady_clock;

	static constexpr float REPORT_EVERY = 5.0f;

	Clock::time_point frameStart;
	Clock::time_point reportedAt;

	Vector<float> total;
	Vector<float> work;

	float lastWork = 0.0f;
	long long counted = 0;

	static float Milliseconds(Clock::time_point from, Clock::time_point to)
	{
		return std::chrono::duration<float, std::milli>(to - from).count();
	}

	static float Percentile(Vector<float> &samples, float fraction)
	{
		if (samples.empty())
		{
			return 0.0f;
		}

		const std::size_t at = std::min(samples.size() - 1,
			static_cast<std::size_t>(std::floor(fraction * static_cast<float>(samples.size()))));

		std::ranges::nth_element(samples, samples.begin() + static_cast<std::ptrdiff_t>(at));

		return samples[at];
	}

	static String Figure(float value)
	{
		return std::to_string(static_cast<int>(std::round(value * 10.0f)) / 10.0f).substr(0, 5);
	}

	void Report(float budget)
	{
		if (total.empty())
		{
			return;
		}

		int late = 0;
		float worst = 0.0f;

		for (float sample : total)
		{
			if (sample > budget * 2.0f)
			{
				late++;
			}

			worst = std::max(worst, sample);
		}

		const String frame = "p50 " + Figure(Percentile(total, 0.50f)) +
							 " p95 " + Figure(Percentile(total, 0.95f)) +
							 " p99 " + Figure(Percentile(total, 0.99f)) +
							 " max " + Figure(worst);

		const String inside = "work p50 " + Figure(Percentile(work, 0.50f)) +
							  " p99 " + Figure(Percentile(work, 0.99f)) +
							  " max " + Figure(work.empty() ? 0.0f : *std::ranges::max_element(work));

		Log::Info("Frames: " + std::to_string(total.size()) + " in " + Figure(REPORT_EVERY) +
				  "s | " + frame + " | " + inside + " | over " + Figure(budget * 2.0f) + "ms: " + std::to_string(late));

		// Read while the engine is still running, and kept when it is stopped
		// rather than killed with a buffer still full.
		std::fflush(stdout);
	}
};
} // namespace TGX

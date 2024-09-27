
#pragma once

#include <random>
#include <iostream>
#include <format>
#include <thread>
#include <memory>
#include <mutex>
#include <csignal>

volatile static bool stop_signal = false;

static void stop(int signal)
{
	stop_signal = true;
}

template <class T>
class Optimizer
{
public:
	Optimizer(T& problem)
	{
		best_solution = problem;
		best_solution.compute();
	}

	T run(const int iterations, const int deviations_allowed)
	{
		std::signal(SIGINT, stop);
		unsigned int n_threads = std::thread::hardware_concurrency();
		std::vector<std::unique_ptr<std::thread>> threads;
		for(int i = 0; i < n_threads; i++)
			threads.emplace_back(std::make_unique<std::thread>(&Optimizer<T>::runner, this, i, iterations, deviations_allowed));
		for(auto& thread : threads)
		{
			thread->join();
			thread.reset();
		}
		return best_solution;
	}

private:

	void runner(const int thread_id, const int iterations, const int deviations_allowed)
	{
		T local_solution;
		T local_best_solution;

		int deviations_remaining = deviations_allowed;

		std::random_device rd;
		std::mt19937 gen(rd());

		{
			const std::lock_guard<std::mutex> lock(solution_mutex);
			local_solution = best_solution;
			local_best_solution = best_solution;
		}

		for(int i = 0; i < iterations && !stop_signal; i++)
		{
			local_solution.random_op(gen);
			local_solution.compute();

			if(local_solution < local_best_solution)
			{
				local_best_solution = local_solution;
				deviations_remaining = deviations_allowed;
			}
			else
			{
				if(deviations_remaining == 0)
				{
					local_solution = local_best_solution;
					deviations_remaining = deviations_allowed;
				}
				else
				{
					deviations_remaining--;
				}
			}

			// Every once in a while, compare the thread solution to the global solution
			if(i % 1000 == 0)
			{
				const std::lock_guard<std::mutex> lock(solution_mutex);

				// Update global solution if we are better
				if(local_best_solution < best_solution)
				{
					std::cout << std::format("Iteration {} / {}. Thread {} got the best solution: {:.4f}", i, iterations, thread_id, local_best_solution.get_metric()) << std::endl;
					best_solution = local_best_solution;
				}

				// Follow the global solution if we are worse
				if(best_solution < local_best_solution)
				{
					std::cout << std::format("Thread {} resets its best solution", thread_id) << std::endl;
					local_best_solution = best_solution;
				}
			}
		}
	}

	T best_solution;
	std::mutex solution_mutex;
};

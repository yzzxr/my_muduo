#include <bits/stdc++.h>

using namespace std::literals::chrono_literals;


using std::cout;
using std::endl;
using std::vector;
using std::queue;
using std::thread;
using std::function;

class A : public std::enable_shared_from_this<A>
{
public:
	A() : x{ 0 } { std::cout << "construct\n"; };
	~A() { std::cout << "deconstruct\n"; }
private:
	int x;
};

std::mutex stdout_lock;
thread_local int x = 0;

void foo()
{
	for (int i = 0; i < 100000000; i++)
		x++;
	{
		std::lock_guard<std::mutex> guard(stdout_lock);
		std::cout << "in thread" << std::this_thread::get_id() << ": x = " << x << '\n';
	}
}


struct ThreadPool
{
private:
	vector<thread> _workers;
	queue<function<void()>> _tasks;

	std::condition_variable _cv;
	std::mutex _mtx;

	std::atomic<bool> _stoped;
public:
	inline ThreadPool(int n) : _stoped{ false }
	{
		for (int i = 0; i < n; i++)
			_workers.emplace_back([this]() {
			while (true)
			{
				function<void()> func;
				{
					std::unique_lock<std::mutex> lock(this->_mtx);
					_cv.wait(lock, [this]() {return this->_stoped || !_tasks.empty();});
					if (_stoped && _tasks.empty())
						break;
					func = std::move(_tasks.front());
					_tasks.pop();
				}
				func();
			}
		});
	}

	inline ~ThreadPool()
	{
		_stoped = true;
		_cv.notify_all();
		for (auto&& worker : _workers)
			worker.join();
	}

	template<typename Func, typename... Args>
	auto commit(Func&& func, Args&&... args)
	{
		if (this->_stoped)
			throw new std::runtime_error("thread pool have stoped!");

		using RT = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<RT()>>(std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

		{
			std::lock_guard<std::mutex> guard(_mtx);
			_tasks.emplace([task](){(*task)();});
		}

		auto result = task->get_future();
		_cv.notify_one();
		return result;
	}

	/// @brief 迪杰斯特拉算法
	vector<int> dijiesitela(vector<vector<int>>& grid)
	{
		int n = grid.size(); 	// 节点号从1开始
		
		vector<int> minDist(n, INT_MAX);
		minDist[1] = 0;
		vector<bool> access(n, false);

		for (int i = 1; i <= n; i++)
		{
			int mid = -1;
			int min_dist = INT_MAX;
			for (int k = 1; k <= n; k++)
				if (minDist[k] < min_dist && !access[k])
					min_dist = minDist[k], mid = k;
			
			if (mid == -1) break;
			access[mid] = true;

			for (int j = 1; j <= n; j++)
				if (!access[j] && grid[mid][j] != INT_MAX && grid[mid][j] + minDist[mid] < minDist[j])
					minDist[j] = grid[mid][j] + minDist[mid];
		}

		return minDist;
	}

	vector<vector<int>> floyd(vector<vector<int>>& grid)
	{
		int n = grid.size();

		constexpr int INF = 1E9 + 7;
		vector<vector<int>> minDist = grid;
		for (int i = 0; i < n; i++)
			for (int j = 0; j < n; j++)
				if (minDist[i][j] == INT_MAX) 
					minDist[i][j] = INF;

		for (int k = 1; k <= n; k++)
			for (int i = 1; i <= n; i++)
				for (int j = 1; j <= n; j++)
					if (minDist[i][k] != INF && minDist[k][j] != INF && minDist[i][k] + minDist[k][j] < minDist[i][j])
						minDist[i][j] = minDist[i][k] + minDist[k][j];
	}
};



int main(int argc, const char* argv[], const char* envp[])
{
	char c = 255;
	ushort b = c;
	cout << b << endl;	
}

#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <limits>

template<typename T>
class ConcurrentQueue
{
public:
	ConcurrentQueue() = default;
	explicit ConcurrentQueue(size_t max_size)
		:_max_size{ max_size } {}
	~ConcurrentQueue() = default;

	template<typename T1>
	void push(T1&& src) {
		std::unique_lock<std::mutex> lk(_mutex);
		_cond_not_full.wait(lk, [this]() {return _data.size() < _max_size;});
		_data.emplace(std::forward<T1>(src));
		lk.unlock();
		_cond_not_empty.notify_one();
	}

	//block until at least one item is available
	T pop() {
		std::unique_lock<std::mutex> lk(_mutex);
		_cond_not_empty.wait(lk, [this]() {return !_data.empty();});
		T result = std::move(_data.front());
		_data.pop();
		lk.unlock();
		_cond_not_full.notify_one();
		return result;
	}

	//return false immediately if the queue is empty
	bool try_pop(T& dst) {
		std::unique_lock<std::mutex> lk(_mutex);
		if (!_data.empty()) {
			dst = std::move(_data.front());
			_data.pop();
			lk.unlock();
			_cond_not_full.notify_one();
			return true;
		}
		else {
			return false;
		}
	}
private:
	std::queue<T> _data;
	size_t _max_size = std::numeric_limits<size_t>::max();
	mutable std::mutex _mutex;
	mutable std::condition_variable _cond_not_empty;
	mutable std::condition_variable _cond_not_full;
};

#pragma once

#include <deque>
#include <vector>
#include <cassert>
#include <deque>
#include <mutex>

/////////////////////////////////////////////////
/**
 * @brief 线程安全队列
 */
template<typename T, typename D = std::deque<T> >
class ThreadQueue
{
public:
    ThreadQueue():_size(0){};

public:

    typedef D queue_type;

    /**
     * @brief 从头部获取数据, 没有数据则等待.
     *
     * @param t 
     * @param millsecond   阻塞等待时间(ms) 
     *                    0 表示不阻塞 
     *                      -1 永久等待
     * @return bool: true, 获取了数据, false, 无数据
     */
    bool pop_front(T& t, size_t millsecond = 0);

    /**
     * @brief 通知等待在队列上面的线程都醒过来
     */
    void notifyT();

    /**
     * @brief 放数据到队列后端. 
     *  
     * @param t
     */
    void push_back(const T& t);

    /**
     * @brief  放数据到队列后端. 
     *  
     * @param vt
     */
    void push_back(const queue_type &qt);
    
    /**
     * @brief  放数据到队列前端. 
     *  
     * @param t
     */
    void push_front(const T& t);

    /**
     * @brief  放数据到队列前端. 
     *  
     * @param vt
     */
    void push_front(const queue_type &qt);

    /**
     * @brief  等到有数据才交换. 
     *  
     * @param q
     * @param millsecond  阻塞等待时间(ms) 
     *                   0 表示不阻塞 
     *                      -1 如果为则永久等待
     * @return 有数据返回true, 无数据返回false
     */
    bool swap(queue_type &q, size_t millsecond = 0);

    /**
     * @brief  队列大小.
     *
     * @return size_t 队列大小
     */
    size_t size() const;

    /**
     * @brief  清空队列
     */
    void clear();

    /**
     * @brief  是否数据为空.
     *
     * @return bool 为空返回true，否则返回false
     */
    bool empty() const;

protected:
    /**
     * 队列
     */
    queue_type          _queue;

    /**
     * 队列长度
     */
    size_t              _size;

private:
	std::mutex		tMutex_;
	std::condition_variable tCondition;
	typedef std::unique_lock<std::mutex> Lock;
};

template<typename T, typename D> bool ThreadQueue<T, D>::pop_front(T& t, size_t millsecond)
{
    Lock lock(tMutex_);

    if (_queue.empty())
    {
        if(millsecond == 0)
        {
            return false;
        }
        if(millsecond == (size_t)-1)
        {
			tCondition.wait(lock);
            //wait();
        }
        else
        {
            //超时了
            //if(!timedWait(millsecond))
			if (tCondition.wait_for(lock, std::chrono::milliseconds(millsecond)) == std::cv_status::timeout)
            {
                return false;
            }
        }
    }

    if (_queue.empty())
    {
        return false;
    }

    t = _queue.front();
    _queue.pop_front();
    assert(_size > 0);
    --_size;


    return true;
}

template<typename T, typename D> void ThreadQueue<T, D>::notifyT()
{
    Lock lock(tMutex_);
    
	tCondition.notify_all();
}

template<typename T, typename D> void ThreadQueue<T, D>::push_back(const T& t)
{
	Lock lock(tMutex_);

	tCondition.notify_one();

    _queue.push_back(t);
    ++_size;
}

template<typename T, typename D> void ThreadQueue<T, D>::push_back(const queue_type &qt)
{
	Lock lock(tMutex_);

    typename queue_type::const_iterator it = qt.begin();
    typename queue_type::const_iterator itEnd = qt.end();
    while(it != itEnd)
    {
        _queue.push_back(*it);
        ++it;
        ++_size;

		tCondition.notify_one();
    }
}

template<typename T, typename D> void ThreadQueue<T, D>::push_front(const T& t)
{
	Lock lock(tMutex_);

	tCondition.notify_one();

    _queue.push_front(t);

    ++_size;
}

template<typename T, typename D> void ThreadQueue<T, D>::push_front(const queue_type &qt)
{
	Lock lock(tMutex_);

    typename queue_type::const_iterator it = qt.begin();
    typename queue_type::const_iterator itEnd = qt.end();
    while(it != itEnd)
    {
        _queue.push_front(*it);
        ++it;
        ++_size;

		tCondition.notify_one();
    }
}

template<typename T, typename D> bool ThreadQueue<T, D>::swap(queue_type &q, size_t millsecond)
{
	Lock lock(tMutex_);

    if (_queue.empty())
    {
        if(millsecond == 0)
        {
            return false;
        }
        if(millsecond == (size_t)-1)
        {
			tCondition.wait(lock);
        }
        else
        {
            //超时了
            if(!tCondition.wait_for(std::chrono::milliseconds(millsecond)))
            {
                return false;
            }
        }
    }

    if (_queue.empty())
    {
        return false;
    }

    q.swap(_queue);
    //_size = q.size();
    _size = _queue.size();

    return true;
}

template<typename T, typename D> size_t ThreadQueue<T, D>::size() const
{
	Lock lock(tMutex_);
    //return _queue.size();
    return _size;
}

template<typename T, typename D> void ThreadQueue<T, D>::clear()
{
	Lock lock(tMutex_);
    _queue.clear();
    _size = 0;
}

template<typename T, typename D> bool ThreadQueue<T, D>::empty() const
{
	Lock lock(tMutex_);
    return _queue.empty();
}


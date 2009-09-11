/*
 * Copyright (C) 2009, Willow Garage, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the names of Stanford University or Willow Garage, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived from
 *     this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ROSCPP_TIMER_MANAGER_H
#define ROSCPP_TIMER_MANAGER_H

#include "ros/forwards.h"
#include "ros/time.h"

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "ros/assert.h"
#include "ros/callback_queue_interface.h"

namespace ros
{

template<class T, class D, class E>
class TimerManager
{
private:
  struct TimerInfo
  {
    int32_t handle;
    D period;

    boost::function<void(const E&)> callback;
    boost::mutex callback_mutex;
    CallbackQueueInterface* callback_queue;

    WallDuration last_cb_duration;

    T last_expected;
    T next_expected;

    T last_real;

    bool removed;

    VoidWPtr tracked_object;
    bool has_tracked_object;

    uint32_t waiting_callbacks;

    // debugging info
    uint32_t total_calls;
  };
  typedef boost::shared_ptr<TimerInfo> TimerInfoPtr;
  typedef boost::weak_ptr<TimerInfo> TimerInfoWPtr;
  typedef std::vector<TimerInfoPtr> V_TimerInfo;

public:
  TimerManager();
  ~TimerManager();

  int32_t add(const D& period, const boost::function<void(const E&)>& callback, CallbackQueueInterface* callback_queue, const VoidPtr& tracked_object);
  void remove(int32_t handle);

  bool hasPending(int32_t handle);

  static TimerManager& global()
  {
    static TimerManager<T, D, E> global;
    return global;
  }

private:
  void threadFunc();

  bool timerCompare(const TimerInfoPtr& lhs, const TimerInfoPtr& rhs);
  TimerInfoPtr findTimer(int32_t handle);

  V_TimerInfo timers_;
  boost::mutex timers_mutex_;

  uint32_t id_counter_;
  boost::mutex id_mutex_;

  bool thread_started_;

  boost::thread thread_;

  bool quit_;

  class TimerQueueCallback : public CallbackInterface
  {
  public:
    TimerQueueCallback(const TimerInfoPtr& info, T last_expected, T last_real, T current_expected)
    : info_(info)
    , last_expected_(last_expected)
    , last_real_(last_real)
    , current_expected_(current_expected)
    , called_(false)
    {
      boost::mutex::scoped_lock lock(info->callback_mutex);
      ++info->waiting_callbacks;
    }

    ~TimerQueueCallback()
    {
      TimerInfoPtr info = info_.lock();
      if (info)
      {
        boost::mutex::scoped_lock lock(info->callback_mutex);
        --info->waiting_callbacks;
      }
    }

    CallResult call()
    {
      TimerInfoPtr info = info_.lock();
      if (!info)
      {
        return Invalid;
      }

      {
        boost::mutex::scoped_lock lock(info->callback_mutex);

        ++info->total_calls;
        called_ = true;

        if (info->removed)
        {
          return Invalid;
        }

        VoidPtr tracked;
        if (info->has_tracked_object)
        {
          tracked = info->tracked_object.lock();
          if (!tracked)
          {
            return Invalid;
          }
        }

        E event;
        event.last_expected = last_expected_;
        event.last_real = last_real_;
        event.current_expected = current_expected_;
        event.current_real = T::now();
        event.profile.last_duration = info->last_cb_duration;

        WallTime cb_start = WallTime::now();
        info->callback(event);
        WallTime cb_end = WallTime::now();
        info->last_cb_duration = cb_end - cb_start;

        info->last_real = event.current_real;
      }

      return Success;
    }

  private:
    TimerInfoWPtr info_;
    T last_expected_;
    T last_real_;
    T current_expected_;

    bool called_;
  };
};

template<class T, class D, class E>
TimerManager<T, D, E>::TimerManager()
: id_counter_(0)
, thread_started_(false)
, quit_(false)
{

}

template<class T, class D, class E>
TimerManager<T, D, E>::~TimerManager()
{
  quit_ = true;
  if (thread_started_)
  {
    thread_.join();
  }
}

template<class T, class D, class E>
bool TimerManager<T, D, E>::timerCompare(const TimerInfoPtr& lhs, const TimerInfoPtr& rhs)
{
  return lhs->next_expected < rhs->next_expected;
}

template<class T, class D, class E>
typename TimerManager<T, D, E>::TimerInfoPtr TimerManager<T, D, E>::findTimer(int32_t handle)
{
  typename V_TimerInfo::iterator it = timers_.begin();
  typename V_TimerInfo::iterator end = timers_.end();
  for (; it != end; ++it)
  {
    if ((*it)->handle == handle)
    {
      return *it;
    }
  }

  return TimerInfoPtr();
}

template<class T, class D, class E>
bool TimerManager<T, D, E>::hasPending(int32_t handle)
{
  boost::mutex::scoped_lock lock(timers_mutex_);
  TimerInfoPtr info = findTimer(handle);

  if (!info)
  {
    return false;
  }

  if (info->has_tracked_object)
  {
    VoidPtr tracked = info->tracked_object.lock();
    if (!tracked)
    {
      return false;
    }
  }

  return info->next_expected <= T::now() || info->waiting_callbacks != 0;
}

template<class T, class D, class E>
int32_t TimerManager<T, D, E>::add(const D& period, const boost::function<void(const E&)>& callback, CallbackQueueInterface* callback_queue, const VoidPtr& tracked_object)
{
  TimerInfoPtr info(new TimerInfo);
  info->period = period;
  info->callback = callback;
  info->callback_queue = callback_queue;
  info->last_expected = T::now();
  info->next_expected = info->last_expected + period;
  info->removed = false;
  info->has_tracked_object = false;
  info->waiting_callbacks = 0;
  info->total_calls = 0;
  if (tracked_object)
  {
    info->tracked_object = tracked_object;
    info->has_tracked_object = true;
  }

  {
    boost::mutex::scoped_lock lock(id_mutex_);
    info->handle = id_counter_++;
  }

  boost::mutex::scoped_lock lock(timers_mutex_);
  timers_.push_back(info);
  std::sort(timers_.begin(), timers_.end(), boost::bind(&TimerManager::timerCompare, this, _1, _2));

  if (!thread_started_)
  {
    thread_ = boost::thread(boost::bind(&TimerManager::threadFunc, this));
    thread_started_ = true;
  }

  return info->handle;
}

template<class T, class D, class E>
void TimerManager<T, D, E>::remove(int32_t handle)
{
  boost::mutex::scoped_lock lock(timers_mutex_);

  typename V_TimerInfo::iterator it = timers_.begin();
  typename V_TimerInfo::iterator end = timers_.end();
  for (; it != end; ++it)
  {
    const TimerInfoPtr& info = *it;
    if (info->handle == handle)
    {
      {
        boost::mutex::scoped_lock lock(info->callback_mutex);
        info->removed = true;
      }
      timers_.erase(it);
      break;
    }
  }
}

template<class T, class D, class E>
void TimerManager<T, D, E>::threadFunc()
{
  while (!quit_)
  {
    T sleep_end;

    {
      boost::mutex::scoped_lock lock(timers_mutex_);

      if (timers_.empty())
      {
        sleep_end = T::now() + D(0.1);
      }
      else
      {
        TimerInfoPtr info = timers_.front();

        while (info->next_expected <= T::now())
        {
          CallbackInterfacePtr cb(new TimerQueueCallback(info, info->last_expected, info->last_real, info->next_expected));
          info->callback_queue->addCallback(cb);

          info->last_expected = info->next_expected;
          info->next_expected += info->period;

          std::sort(timers_.begin(), timers_.end(), boost::bind(&TimerManager::timerCompare, this, _1, _2));

          info = timers_.front();
        }

        sleep_end = info->next_expected;
      }
    }

    T::sleepUntil(sleep_end);
  }
}

}

#endif

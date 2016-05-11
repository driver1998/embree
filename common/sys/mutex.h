// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "platform.h"
#include "intrinsics.h"
#include "atomic.h"

namespace embree
{
  /*! system mutex */
  class MutexSys {
    friend struct ConditionImplementation;
  public:
    MutexSys( void );
    ~MutexSys( void );

    void lock( void );
    bool try_lock( void );
    void unlock( void );

  protected:
    void* mutex;
  };

  /*! spinning mutex */
  class AtomicMutex
  {
  public:
 
    AtomicMutex ()
      : flag(false) {}

    __forceinline bool isLocked() {
      return flag.load();
    }

    __forceinline void lock()
    {
      while (true) 
      {
        while (flag.load()) 
        {
          _mm_pause(); 
          _mm_pause();
        }
        
        bool expected = false;
        if (flag.compare_exchange_strong(expected,true))
          break;
      }
    }
    
    __forceinline bool try_lock()
    {
      bool expected = false;
      if (flag.load() != expected) return false;
      return flag.compare_exchange_strong(expected,true);
    }

    __forceinline void unlock() {
      flag.store(false);
    }
    
    __forceinline void wait_until_unlocked() 
    {
      __memory_barrier();
      while(flag.load())
      {
        _mm_pause(); 
        _mm_pause();
      }
      __memory_barrier();
    }

    __forceinline void reset(int i = 0) 
    {
      __memory_barrier();
      flag.store(i);
      __memory_barrier();
    }

  public:
    std__atomic<bool> flag;
  };

  /*! safe mutex lock and unlock helper */
  template<typename Mutex> class Lock {
  public:
    Lock (Mutex& mutex) : mutex(mutex) { mutex.lock(); }
    ~Lock() { mutex.unlock(); }
  protected:
    Mutex& mutex;
  };

  /*! safe mutex try_lock and unlock helper */
  template<typename Mutex> class TryLock {
  public:
    TryLock (Mutex& mutex) : mutex(mutex), locked(mutex.try_lock()) {}
    ~TryLock() { if (locked) mutex.unlock(); }
    __forceinline bool isLocked() const { return locked; }
  protected:
    Mutex& mutex;
    bool locked;
  };

  /*! safe mutex try_lock and unlock helper */
  template<typename Mutex> class AutoUnlock {
  public:
    AutoUnlock (Mutex& mutex) : mutex(mutex), locked(false) {}
    ~AutoUnlock() { if (locked) mutex.unlock(); }
    __forceinline void lock() { locked = true; mutex.lock(); }
    __forceinline bool isLocked() const { return locked; }
  protected:
    Mutex& mutex;
    bool locked;
  };
}

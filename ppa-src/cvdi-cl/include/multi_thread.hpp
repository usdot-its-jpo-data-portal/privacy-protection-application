/*******************************************************************************
 * Copyright 2018 UT-Battelle, LLC
 * All rights reserved
 * Route Sanitizer, version 0.9
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For issues, question, and comments, please submit a issue via GitHub.
 *******************************************************************************/
#ifndef MULTI_THREAD_HPP
#define MULTI_THREAD_HPP

#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace MultiThread {

    template <typename T>
    class SharedQueue
    {
        public:
            T pop() {
                std::unique_lock<std::mutex> mlock(mutex_);
                
                while (queue_.empty()) {
                    cond_.wait(mlock);
                }
                
                auto val = queue_.front();
                
                queue_.pop();
                
                return val;
            }
    
            void pop(T& item) {
                std::unique_lock<std::mutex> mlock(mutex_);
                
                while (queue_.empty()) {
                    cond_.wait(mlock);
                }
    
                item = queue_.front();
                queue_.pop();
            }
    
            void push(const T& item) {
                std::unique_lock<std::mutex> mlock(mutex_);
                queue_.push(item);
                mlock.unlock();
                cond_.notify_one();
            }
      
            SharedQueue()=default;
            SharedQueue(const SharedQueue&) = delete;            // disable copying
            SharedQueue& operator=(const SharedQueue&) = delete; // disable assignment
      
        private:
            std::queue<T> queue_;
            std::mutex mutex_;
            std::condition_variable cond_;
    };

    template <typename T>
    class Parallel
    {

        public:
            void Start(unsigned n_threads) {
                // Figure out how many threads we can actually use.
                unsigned n_used_threads = n_threads;
                
                // Call the initialize routine for the subclass.
                Init(n_threads);

                // list of threads
                std::vector<std::thread> threads(n_threads);
                // pointer to a shred queue
                SharedQueue<std::shared_ptr<T>>* q_ptr = new SharedQueue<std::shared_ptr<T>>;
                // Start each thread.
                // the thread's queue, and start each thread.
                for (unsigned i = 0; i < n_threads; ++i)
                {
                    threads[i] = std::thread(&Parallel::Thread, this, i, q_ptr);
                }

                std::shared_ptr<T> item_ptr = nullptr;

                // Get the items from the subclass.
                // For each item get the item size and add it the queue of the
                // thread with the least amount of load.
                for (std::shared_ptr<T> item_ptr = NextItem(); item_ptr != nullptr; item_ptr = NextItem()) 
                {
                    q_ptr->push(item_ptr);
                } 

                // Join all the threads.
                // Pass a null pointer to tell the threads not to expect 
                // anymore items.
                // Clean up the queue memory.
                for (unsigned i = 0; i < n_threads; ++i) {
                    q_ptr->push(nullptr);
                }

                for (unsigned i = 0; i < n_threads; ++i) {
                    threads[i].join();
                }

                delete q_ptr; 

                // Call the subclass close method.
                Close();
            }

            virtual void Thread(unsigned thread_number, SharedQueue<std::shared_ptr<T>>* q) = 0;
            virtual void Init(unsigned n_threads) = 0;
            virtual void Close(void) = 0;
            virtual std::shared_ptr<T> NextItem(void) = 0;
    };
}

#endif

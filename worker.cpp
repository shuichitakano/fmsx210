/*
 * author : Shuichi TAKANO
 * since  : Sat Jan 30 2021 18:17:26
 */

#include "worker.h"
#include <vector>
#include <stdio.h>

#include <sys/lock.h>
#include <bsp.h>

namespace
{

    class Worker
    {
        std::vector<std::function<void()>> workloads_;
        _lock_t lock_;

    public:
        Worker()
        {
            _lock_init(&lock_);
        }

        void loop()
        {
            printf("start worker.\n");
            while (1)
            {
                _lock_acquire(&lock_);
                if (!workloads_.empty())
                {
                    auto f = std::move(workloads_.front());
                    workloads_.erase(workloads_.begin());
                    _lock_release(&lock_);

                    f();
                }
                else
                {
                    _lock_release(&lock_);
                }
            }
        }

        void append(std::function<void()> w)
        {
            _lock_acquire(&lock_);
            workloads_.push_back(w);
            _lock_release(&lock_);
        }

        static Worker &instance()
        {
            static Worker worker;
            return worker;
        }
    };

} // namespace

void startWorker()
{
    auto worker = [](void *) -> int {
        Worker::instance().loop();
        return {};
    };

    register_core1(worker, nullptr);
}

void addWorkload(std::function<void()> &&f)
{
    Worker::instance().append(f);
}

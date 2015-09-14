#ifndef UMS_UTILS_RUNNABLE_H_
#define UMS_UTILS_RUNNABLE_H_

#include <atomic>
#include <thread>

/**
 * Java-like interface for std::thread.
 * @author zsy
 */
class Runnable
{
public:
    Runnable();
    virtual ~Runnable(); 

    /**
     * Set _stop to false and destroy the thread.
     * MUST stop the thread when called.
     */
    void stop();
    void start(); 
    /**
     * Wait function Runnable#run to stop.
     * @see Runnable#run
     */
    void join();
    /**
     * If it is still running and not detached.
     */
    bool joinable(); 

protected:
    virtual void run() = 0;

    /**
     * true if stop is called.
     */
    std::atomic<bool> _stop;

private:
    /**
     * using a pointer to prevent the inconsistency 
     * when a runnable is distructing
     */
    std::thread* _thread;
};

#endif  // UMS_UTILS_RUNNABLE_H_

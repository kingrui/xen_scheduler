#include "runnable.h"

#include <cassert>
#include <iostream>

using namespace std;

Runnable::Runnable():
    _stop(true), _thread(nullptr) 
{}


Runnable::~Runnable()
{
    /*if (!_stop) {
        try {
            stop();
        } catch (const exception& e) {
            LOG(LogLevel::warn) << "Thread stop exception " << e.what() << endl;
        }
    }*/     
    delete _thread;
}

void Runnable::join()
{
    _thread->join();
}

bool Runnable::joinable() 
{
    return _thread != nullptr && _thread->joinable();
}

void Runnable::start()
{
    if (!_stop) {
        cout << "Runnable::start: "
            << "Thread is running" << endl;
        return;
    }
    if (_thread != nullptr) {
        delete _thread;
        _thread = nullptr;
    }
    _stop = false;
    _thread = new thread(&Runnable::run, this);
}

void Runnable::stop()
{
    if (_stop) {
        return;
    }
    _stop = true;
    if (_thread != nullptr && _thread->joinable()) {
        _thread->detach();
    } 
}


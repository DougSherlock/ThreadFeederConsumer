// ThreadFeederConsumer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <mutex>
#include <sstream>
#include <vector>
#include <queue>

using namespace std;

class Log
{
public:
    void Out(const char* function, const string msg) {
        unique_lock<mutex> lock(_mutex);
        cout << function << " " << msg << endl;
    }
    void Out(const char* function, const stringstream& msg) {
        Out(function, msg.str());
    }
private:
    mutex _mutex;
};

Log g_log;

class ThreadData
{
public:
    ThreadData() : _done(false) {}
    queue<int> _numbers;
    mutex _mutex;
    bool _done;
    condition_variable _condition;
};

void FeedFunc(ThreadData& data)
{
    g_log.Out(__FUNCTION__, "started");
    const int MAX_FEED = 100;
    for (int val = 0; val < MAX_FEED; ++val)
    {
        unique_lock<mutex> lock(data._mutex);
        g_log.Out(__FUNCTION__, to_string(val));
        data._numbers.push(val);
    }

    {
        unique_lock<mutex> lock(data._mutex);
        g_log.Out(__FUNCTION__, "setting data._done to true");
        data._done = true;
    }

    g_log.Out(__FUNCTION__, "done");
}

void ConsFunc(ThreadData& data)
{
    g_log.Out(__FUNCTION__, "started");
    do 
    {
        unique_lock<mutex> lock(data._mutex);
        if (data._numbers.size() > 0)
        {
            int val = data._numbers.front();
            data._numbers.pop();
            stringstream msg;
            msg << val; // << " queue size:" << data._numbers.size() << " data._done:" << data._done;
            g_log.Out(__FUNCTION__, msg);
        }
        if (data._numbers.size() == 0 && data._done)
        {
            g_log.Out(__FUNCTION__, "quitting loop");
            break;
        }
    } while (true);


    g_log.Out(__FUNCTION__, "done");
}


void FeedFuncSync(ThreadData& data)
{
    g_log.Out(__FUNCTION__, "started");
    const int MAX_FEED = 100;
    for (int val = 0; val < MAX_FEED; ++val)
    {
        unique_lock<mutex> lock(data._mutex);
        data._condition.wait(lock, [&data] { return data._numbers.size() == 0; }); //if (data._numbers.size() > 0)
        if (data._numbers.size() == 0) 
        {
            g_log.Out(__FUNCTION__, to_string(val));
            data._numbers.push(val);
            data._condition.notify_one();
        }
    }

    {
        unique_lock<mutex> lock(data._mutex);
        data._condition.wait(lock, [&data] { return data._numbers.size() == 0; }); //if (data._numbers.size() > 0)
        if (data._numbers.size() == 0)
        {
            data._done = true;
            g_log.Out(__FUNCTION__, "setting data._done to true");
            data._condition.notify_one();
        }
    }

    g_log.Out(__FUNCTION__, "done");
}

void ConsFuncSync(ThreadData& data)
{
    g_log.Out(__FUNCTION__, "started");
    do
    {
        unique_lock<mutex> lock(data._mutex);
        data._condition.wait(lock, [&data] { return data._numbers.size() > 0 || data._done; }); //if (data._numbers.size() > 0)
        if (data._numbers.size() > 0)
        {
            int val = data._numbers.front();
            data._numbers.pop();
            stringstream msg;
            msg << val; // << " queue size:" << data._numbers.size() << " data._done:" << data._done;
            g_log.Out(__FUNCTION__, msg);
            data._condition.notify_one();
        }
        if (data._numbers.size() == 0 && data._done)
        {
            g_log.Out(__FUNCTION__, "quitting loop");
            break;
        }
    } while (true);


    g_log.Out(__FUNCTION__, "done");
}

int main()
{
    g_log.Out(__FUNCTION__, "started");
    ThreadData data;
    g_log.Out(__FUNCTION__, "creating threads");
    thread feederThread(/*FeedFunc*/FeedFuncSync, ref(data));
    thread consumerThread(/*ConsFunc*/ConsFuncSync, ref(data));
    g_log.Out(__FUNCTION__, "waiting for threads to finish");
    consumerThread.join();
    feederThread.join();
    g_log.Out(__FUNCTION__, "threads have finished");
    int n = getchar();         
}


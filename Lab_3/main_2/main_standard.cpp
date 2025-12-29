#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <random>
#include <limits>

using namespace std;

const auto time_for_Sleep = chrono::milliseconds(5);

mutex startMutex;
mutex arrayMutex;
vector<unique_ptr<condition_variable>> continueCVs;
vector<unique_ptr<condition_variable>> stopCVs;
vector<unique_ptr<mutex>> continueMutexes;
vector<unique_ptr<mutex>> stopMutexes;
vector<bool> threadStopped;
vector<bool> threadSignaled;
atomic<int> activeThreads;

struct ThreadArgs {
    vector<int>& arr;
    int index;
};

void marker(ThreadArgs* args) {
    vector<int>& arr = args->arr;
    int i = args->index;

   
    {
        unique_lock<mutex> lock(startMutex);
    }

    mt19937 rng(i + time(nullptr));
    uniform_int_distribution<int> dist(0, arr.size() - 1);

    int count = 0;
    bool working = true;

    while (working) {
        int b = dist(rng);

        {
            unique_lock<mutex> lock(arrayMutex);

            if (arr[b] == 0) {
                this_thread::sleep_for(time_for_Sleep);
                arr[b] = i + 1; 
                this_thread::sleep_for(time_for_Sleep);
                count++;
            }
            else {
                cout << "Thread number: " << i + 1 << endl;
                cout << "Number of marked elements: " << count << endl;
                cout << "It is not possible to mark an element with an index: " << b << endl;

                
                {
                    unique_lock<mutex> lock(*stopMutexes[i]);
                    threadSignaled[i] = true;
                    stopCVs[i]->notify_one();
                }

               
                {
                    unique_lock<mutex> lock(*continueMutexes[i]);
                    continueCVs[i]->wait(lock, [i]() {
                        return threadSignaled[i] == false;
                        });
                }

                if (threadStopped[i]) {
                    working = false;
                   
                    unique_lock<mutex> lock2(arrayMutex);
                    for (int j = 0; j < arr.size(); j++) {
                        if (arr[j] == i + 1) {
                            arr[j] = 0;
                        }
                    }
                }
            }
        }
    }

    activeThreads--;
}

int main() {
    int n = 0;
    int kol = 0;
    vector<int> arr;
    vector<thread> threads;
    vector<unique_ptr<ThreadArgs>> threadArgs;

    try {
        cout << "Enter the size of the array: ";
        cin >> n;
        if (cin.fail() || n <= 0) {
            throw runtime_error("Incorrect array size.");
        }

        arr.resize(n, 0);

        cout << "Enter the number of threads: ";
        cin >> kol;
        if (cin.fail() || kol <= 0) {
            throw runtime_error("Incorrect number of threads.");
        }

        activeThreads = kol;

        
        continueCVs.resize(kol);
        stopCVs.resize(kol);
        continueMutexes.resize(kol);
        stopMutexes.resize(kol);
        threadStopped.resize(kol, false);
        threadSignaled.resize(kol, false);

        for (int i = 0; i < kol; ++i) {
            continueCVs[i] = make_unique<condition_variable>();
            stopCVs[i] = make_unique<condition_variable>();
            continueMutexes[i] = make_unique<mutex>();
            stopMutexes[i] = make_unique<mutex>();
        }

       
        startMutex.lock();

        
        for (int i = 0; i < kol; ++i) {
            threadArgs.push_back(make_unique<ThreadArgs>(ThreadArgs{ arr, i }));
            threads.emplace_back(marker, threadArgs.back().get());
        }

        
        startMutex.unlock();

        while (activeThreads > 0) {
            
            bool anySignaled = false;
            int signaledIndex = -1;

            for (int i = 0; i < kol; i++) {
                if (!threadStopped[i]) {
                    unique_lock<mutex> lock(*stopMutexes[i]);
                    if (stopCVs[i]->wait_for(lock, chrono::milliseconds(100),
                        [i]() { return threadSignaled[i]; })) {
                        anySignaled = true;
                        signaledIndex = i;
                        break;
                    }
                }
            }

            if (!anySignaled) {
               
                bool allMarked = true;
                {
                    lock_guard<mutex> lock(arrayMutex);
                    for (int val : arr) {
                        if (val == 0) {
                            allMarked = false;
                            break;
                        }
                    }
                }

                if (allMarked) {
                    cout << "No zero elements left. Stopping all threads." << endl;
                    for (int i = 0; i < kol; i++) {
                        if (!threadStopped[i]) {
                            threadStopped[i] = true;
                            threadSignaled[i] = false;
                            continueCVs[i]->notify_one();
                        }
                    }
                    break;
                }
                continue;
            }

            
            {
                lock_guard<mutex> lock(arrayMutex);
                cout << "Array contents: ";
                for (int val : arr) {
                    cout << val << " ";
                }
                cout << endl;
            }

            
            int konec;
            cout << "Enter the thread number to complete: ";
            cin >> konec;

            if (cin.fail() || konec < 1 || konec > kol) {
                cout << "Incorrect thread number. Continuing all threads." << endl;
                for (int i = 0; i < kol; i++) {
                    if (!threadStopped[i] && threadSignaled[i]) {
                        threadSignaled[i] = false;
                        continueCVs[i]->notify_one();
                    }
                }
                continue;
            }

            int threadIndex = konec - 1;

            if (threadStopped[threadIndex]) {
                cout << "Thread " << konec << " is already stopped." << endl;
                for (int i = 0; i < kol; i++) {
                    if (!threadStopped[i] && threadSignaled[i]) {
                        threadSignaled[i] = false;
                        continueCVs[i]->notify_one();
                    }
                }
                continue;
            }

            
            threadStopped[threadIndex] = true;
            threadSignaled[threadIndex] = false;
            continueCVs[threadIndex]->notify_one();

            
            if (threads[threadIndex].joinable()) {
                threads[threadIndex].join();
            }

            
            {
                lock_guard<mutex> lock(arrayMutex);
                cout << "Array contents after stopping thread " << konec << ": ";
                for (int val : arr) {
                    cout << val << " ";
                }
                cout << endl;
            }

            
            for (int i = 0; i < kol; i++) {
                if (!threadStopped[i] && threadSignaled[i]) {
                    threadSignaled[i] = false;
                    continueCVs[i]->notify_one();
                }
            }
        }

        cout << "All threads have been completed." << endl;

        
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
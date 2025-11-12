#include <iostream>
#include <vector>
#include <future>
#include <thread>
#include <atomic>
#include <mutex>

int THREAD_LIMIT = std::thread::hardware_concurrency(); // Максимум потоков
std::atomic<int> activeThreads(0); // Счётчик активных потоков
std::mutex threadMutex;

void merge(std::vector<int>& arr, int l, int m, int r) {
    int nl = m - l + 1;
    int nr = r - m;

    std::vector<int> left(nl), right(nr);
    for (int i = 0; i < nl; ++i) left[i] = arr[l + i];
    for (int j = 0; j < nr; ++j) right[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = l;
    while (i < nl && j < nr) {
        arr[k++] = (left[i] <= right[j]) ? left[i++] : right[j++];
    }
    while (i < nl) arr[k++] = left[i++];
    while (j < nr) arr[k++] = right[j++];
}

void mergeSort(std::vector<int>& arr, int l, int r) {
    if (l >= r) return;
    int m = l + (r - l) / 2;

    std::future<void> leftFuture, rightFuture;
    bool leftAsync = false, rightAsync = false;

    {
        std::lock_guard<std::mutex> lock(threadMutex);
        if (activeThreads < THREAD_LIMIT) {
            activeThreads++;
            leftFuture = std::async(std::launch::async, mergeSort, std::ref(arr), l, m);
            leftAsync = true;
        }
    }

    {
        std::lock_guard<std::mutex> lock(threadMutex);
        if (activeThreads < THREAD_LIMIT) {
            activeThreads++;
            rightFuture = std::async(std::launch::async, mergeSort, std::ref(arr), m + 1, r);
            rightAsync = true;
        }
    }

    if (leftAsync) {
        leftFuture.get();
        activeThreads--;
    }
    else {
        mergeSort(arr, l, m);
    }

    if (rightAsync) {
        rightFuture.get();
        activeThreads--;
    }
    else {
        mergeSort(arr, m + 1, r);
    }

    merge(arr, l, m, r);
}

int main() {
    std::vector<int> data = { 38, 27, 43, 3, 9, 82, 10, 56, 29, 71, 4, 99, 12, 65, 33 };
    mergeSort(data, 0, data.size() - 1);

    for (int num : data)
        std::cout << num << " ";
    std::cout << std::endl;

    return 0;
}
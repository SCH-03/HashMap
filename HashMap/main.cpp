#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "StudentHashMap.h"

using Clock = std::chrono::steady_clock;


// ============================================================
// Test Configuration
// ============================================================

constexpr std::size_t ElementCount = 200'000;
constexpr std::size_t FindCount = 1'000'000;


// 최적화로 Find 코드가 사라지는 것을 방지하기 위한 값
std::uint64_t g_Checksum = 0;


// ============================================================
// Benchmark Result
// ============================================================

struct BenchmarkResult
{
    double InsertMs = 0.0;
    double FindHitMs = 0.0;
    double FindMissMs = 0.0;
    double RemoveMs = 0.0;

    std::size_t FinalSize = 0;
};


// ============================================================
// Time Measurement
// ============================================================

template <typename Func>
double MeasureMilliseconds(Func&& func)
{
    const auto begin = Clock::now();

    func();

    const auto end = Clock::now();

    return std::chrono::duration<double, std::milli>(
        end - begin
    ).count();
}


// ============================================================
// Key Generators
// ============================================================

// ------------------------------------------------------------
// Case 1 : Random
//
// 1 ~ ElementCount까지의 고유한 Key를 만든 뒤 섞는다.
// ------------------------------------------------------------

std::vector<int> CreateRandomKeys(std::size_t count)
{
    std::vector<int> keys(count);

    std::iota(
        keys.begin(),
        keys.end(),
        1
    );

    std::mt19937 rng(12345);

    std::shuffle(
        keys.begin(),
        keys.end(),
        rng
    );

    return keys;
}


// ------------------------------------------------------------
// Case 2 : Sequential
//
// 1, 2, 3, 4, 5 ...
// ------------------------------------------------------------

std::vector<int> CreateSequentialKeys(std::size_t count)
{
    std::vector<int> keys(count);

    std::iota(
        keys.begin(),
        keys.end(),
        1
    );

    return keys;
}


// ------------------------------------------------------------
// Case 3 : Pattern x16
//
// 16, 32, 48, 64 ...
//
// 하위 비트가 일정한 패턴을 가진다.
// Hash/Bucket 설계에 따라 Collision이 크게 증가할 수 있다.
// ------------------------------------------------------------

std::vector<int> CreatePatternKeys(std::size_t count)
{
    std::vector<int> keys;

    keys.reserve(count);

    for (std::size_t i = 1; i <= count; ++i)
    {
        keys.push_back(
            static_cast<int>(i * 16)
        );
    }

    return keys;
}


// ============================================================
// Find Hit Keys
// ============================================================

std::vector<int> CreateHitKeys(
    const std::vector<int>& keys,
    std::size_t count)
{
    std::vector<int> result;

    result.reserve(count);

    std::mt19937 rng(54321);

    std::uniform_int_distribution<std::size_t> dist(
        0,
        keys.size() - 1
    );

    for (std::size_t i = 0; i < count; ++i)
    {
        result.push_back(
            keys[dist(rng)]
        );
    }

    return result;
}


// ============================================================
// Find Miss Keys
// ============================================================
//
// 테스트 데이터와 겹치지 않도록 큰 음수를 사용한다.
// ============================================================

std::vector<int> CreateMissKeys(std::size_t count)
{
    std::vector<int> result;

    result.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
    {
        result.push_back(
            -1 - static_cast<int>(i)
        );
    }

    return result;
}


// ============================================================
// StudentHashMap Benchmark
// ============================================================

BenchmarkResult BenchmarkStudent(
    const std::vector<int>& keys,
    const std::vector<int>& hitKeys,
    const std::vector<int>& missKeys)
{
    StudentHashMap map;

    BenchmarkResult result;


    // --------------------------------------------------------
    // Insert
    // --------------------------------------------------------

    result.InsertMs = MeasureMilliseconds([&]()
        {
            for (int key : keys)
            {
                map.Insert(
                    key,
                    key
                );
            }
        });


    // --------------------------------------------------------
    // Find Hit
    // --------------------------------------------------------

    result.FindHitMs = MeasureMilliseconds([&]()
        {
            std::uint64_t checksum = 0;

            for (int key : hitKeys)
            {
                int* value = map.Find(key);

                if (value != nullptr)
                {
                    checksum += static_cast<std::uint64_t>(*value);
                }
            }

            g_Checksum += checksum;
        });


    // --------------------------------------------------------
    // Find Miss
    // --------------------------------------------------------

    result.FindMissMs = MeasureMilliseconds([&]()
        {
            std::uint64_t checksum = 0;

            for (int key : missKeys)
            {
                int* value = map.Find(key);

                if (value != nullptr)
                {
                    checksum += static_cast<std::uint64_t>(*value);
                }
            }

            g_Checksum += checksum;
        });


    // --------------------------------------------------------
    // Remove
    //
    // 전체 Element 중 절반 삭제
    // --------------------------------------------------------

    result.RemoveMs = MeasureMilliseconds([&]()
        {
            for (std::size_t i = 0;
                i < keys.size();
                i += 2)
            {
                map.Remove(keys[i]);
            }
        });


    result.FinalSize = map.Size();

    return result;
}


// ============================================================
// std::unordered_map Benchmark
// ============================================================

BenchmarkResult BenchmarkSTL(
    const std::vector<int>& keys,
    const std::vector<int>& hitKeys,
    const std::vector<int>& missKeys)
{
    std::unordered_map<int, int> map;

    BenchmarkResult result;


    // StudentHashMap과 달리 STL에는 reserve가 존재한다.
    //
    // 실험 목적에 따라 이 줄을 제거하고 비교해도 된다.
    map.reserve(keys.size());


    // --------------------------------------------------------
    // Insert
    // --------------------------------------------------------

    result.InsertMs = MeasureMilliseconds([&]()
        {
            for (int key : keys)
            {
                map.insert_or_assign(
                    key,
                    key
                );
            }
        });


    // --------------------------------------------------------
    // Find Hit
    // --------------------------------------------------------

    result.FindHitMs = MeasureMilliseconds([&]()
        {
            std::uint64_t checksum = 0;

            for (int key : hitKeys)
            {
                auto iter = map.find(key);

                if (iter != map.end())
                {
                    checksum +=
                        static_cast<std::uint64_t>(iter->second);
                }
            }

            g_Checksum += checksum;
        });


    // --------------------------------------------------------
    // Find Miss
    // --------------------------------------------------------

    result.FindMissMs = MeasureMilliseconds([&]()
        {
            std::uint64_t checksum = 0;

            for (int key : missKeys)
            {
                auto iter = map.find(key);

                if (iter != map.end())
                {
                    checksum +=
                        static_cast<std::uint64_t>(iter->second);
                }
            }

            g_Checksum += checksum;
        });


    // --------------------------------------------------------
    // Remove
    // --------------------------------------------------------

    result.RemoveMs = MeasureMilliseconds([&]()
        {
            for (std::size_t i = 0;
                i < keys.size();
                i += 2)
            {
                map.erase(keys[i]);
            }
        });


    result.FinalSize = map.size();

    return result;
}


// ============================================================
// Result Output
// ============================================================

void PrintResult(
    const std::string& testName,
    const BenchmarkResult& student,
    const BenchmarkResult& stl)
{
    std::cout
        << "\n============================================================\n";

    std::cout
        << "Test Case : "
        << testName
        << '\n';

    std::cout
        << "============================================================\n";

    std::cout
        << "Operation\tStudentHashMap\tstd::unordered_map\n";

    std::cout
        << "------------------------------------------------------------\n";

    std::cout
        << "Insert\t\t"
        << student.InsertMs
        << " ms\t\t"
        << stl.InsertMs
        << " ms\n";

    std::cout
        << "Find Hit\t"
        << student.FindHitMs
        << " ms\t\t"
        << stl.FindHitMs
        << " ms\n";

    std::cout
        << "Find Miss\t"
        << student.FindMissMs
        << " ms\t\t"
        << stl.FindMissMs
        << " ms\n";

    std::cout
        << "Remove\t\t"
        << student.RemoveMs
        << " ms\t\t"
        << stl.RemoveMs
        << " ms\n";

    std::cout
        << "Final Size\t"
        << student.FinalSize
        << "\t\t"
        << stl.FinalSize
        << '\n';
}


// ============================================================
// Single Test Case
// ============================================================

void RunTestCase(
    const std::string& testName,
    const std::vector<int>& keys)
{
    const std::vector<int> hitKeys =
        CreateHitKeys(
            keys,
            FindCount
        );

    const std::vector<int> missKeys =
        CreateMissKeys(
            FindCount
        );


    const BenchmarkResult studentResult =
        BenchmarkStudent(
            keys,
            hitKeys,
            missKeys
        );


    const BenchmarkResult stlResult =
        BenchmarkSTL(
            keys,
            hitKeys,
            missKeys
        );


    PrintResult(
        testName,
        studentResult,
        stlResult
    );
}


// ============================================================
// Main
// ============================================================

int main()
{
    std::cout
        << "Element Count : "
        << ElementCount
        << '\n';

    std::cout
        << "Find Count    : "
        << FindCount
        << '\n';


    // ========================================================
    // Case 1 : Random
    // ========================================================

    RunTestCase(
        "Random",
        CreateRandomKeys(ElementCount)
    );


    // ========================================================
    // Case 2 : Sequential
    // ========================================================

    RunTestCase(
        "Sequential",
        CreateSequentialKeys(ElementCount)
    );


    // ========================================================
    // Case 3 : Pattern x16
    // ========================================================

    RunTestCase(
        "Pattern x16",
        CreatePatternKeys(ElementCount)
    );


    std::cout
        << "\nChecksum : "
        << g_Checksum
        << '\n';

    return 0;
}
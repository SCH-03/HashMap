#pragma once

#include <cstddef>
#include <cstdint>
#include <list>
#include <vector>

class StudentHashMap
{
public:
    StudentHashMap();
    ~StudentHashMap();

    StudentHashMap(const StudentHashMap&) = delete;
    StudentHashMap& operator=(const StudentHashMap&) = delete;

public:
    // 같은 Key가 이미 존재하면 Value를 갱신한다.
    void Insert(int key, int value);

    // Key가 존재하면 Value의 주소 반환
    // 존재하지 않으면 nullptr
    int* Find(int key);
    const int* Find(int key) const;

    // 삭제 성공 true
    // 존재하지 않으면 false
    bool Remove(int key);

    void Clear();

    std::size_t Size() const;

private:
    // ============================================
    // 자유롭게 설계
    // ============================================

    struct Entry
    {
        int Key;
        int Value;
    };

    // 초기 Buket 수
    static constexpr std::size_t InitialBucketCount = 16;


    std::vector<std::list<Entry>> m_Buckets;
    std::size_t m_Size;

    static std::size_t Hash(int key);
    std::size_t BucketIndex(int key) const;

    void Rehash(std::size_t newBucketCount);
    
};
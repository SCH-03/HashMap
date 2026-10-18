#include "StudentHashMap.h"

#include <stdexcept>

StudentHashMap::StudentHashMap()
    : m_Buckets(InitialBucketCount),
    m_Size(0)
{
}

StudentHashMap::~StudentHashMap() = default;

void StudentHashMap::Insert(int key, int value)
{
    // 중복 키는 원소 수를 늘리지 않고 값만 갱신한다.
    if (int* existing = Find(key))
    {
        *existing = value;
        return;
    }

    // 새 원소 삽입 후 Load Factor가 1.0을 초과한다면 확장한다.
    if (m_Size >= m_Buckets.size())
    {
        Rehash(m_Buckets.size() * 2);
    }

    // Rehash로 버킷 수가 바뀔 수 있으므로 확장 후 계산한다.
    const std::size_t index = BucketIndex(key);

    m_Buckets[index].push_back(Entry{ key, value });

    // 노드 생성에 성공한 뒤 개수를 증가시킨다.
    ++m_Size;
}

int* StudentHashMap::Find(int key)
{
    auto& bucket = m_Buckets[BucketIndex(key)];

    for (auto& entry : bucket)
    {
        if (entry.Key == key)
        {
            return &entry.Value;
        }
    }

    return nullptr;
}

const int* StudentHashMap::Find(int key) const
{
    const auto& bucket = m_Buckets[BucketIndex(key)];

    for (const auto& entry : bucket)
    {
        if (entry.Key == key)
        {
            return &entry.Value;
        }
    }

    return nullptr;
}

bool StudentHashMap::Remove(int key)
{
    auto& bucket = m_Buckets[BucketIndex(key)];

    for (auto iter = bucket.begin();
        iter != bucket.end();
        ++iter)
    {
        if (iter->Key == key)
        {
            bucket.erase(iter);
            --m_Size;

            return true;
        }
    }

    return false;
}

void StudentHashMap::Clear()
{
    for (auto& bucket : m_Buckets)
    {
        bucket.clear();
    }

    // 버킷은 유지하고 저장된 원소만 제거한다.
    m_Size = 0;
}

std::size_t StudentHashMap::Size() const
{
    return m_Size;
}

// 해쉬함수
std::size_t StudentHashMap::Hash(int key)
{
    // 음수 키도 부호 없는 정수로 변환하여 처리한다.
    std::uint32_t hash = static_cast<std::uint32_t>(key);

    // 규칙적인 키에서도 버킷 분산을 개선하도록 비트를 섞는다.
    hash ^= hash >> 16;
    hash *= 0x7feb352du;
    hash ^= hash >> 15;
    hash *= 0x846ca68bu;
    hash ^= hash >> 16;

    return static_cast<std::size_t>(hash);
}

std::size_t StudentHashMap::BucketIndex(int key) const
{
    return Hash(key) % m_Buckets.size();
}

void StudentHashMap::Rehash(std::size_t newBucketCount)
{
    std::vector<std::list<Entry>> newBuckets(newBucketCount);

    for (auto& bucket : m_Buckets)
    {
        while (!bucket.empty())
        {
            auto iter = bucket.begin();

            const std::size_t newIndex =
                Hash(iter->Key) % newBucketCount;

            // 노드를 새로 할당하지 않고 새 버킷으로 옮긴다.
            newBuckets[newIndex].splice(
                newBuckets[newIndex].end(),
                bucket,
                iter
            );
        }
    }

    m_Buckets.swap(newBuckets);
}
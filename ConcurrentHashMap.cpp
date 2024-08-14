#include <iostream>
#include <vector>
#include <list>
#include <mutex>
#include <functional>
#include <shared_mutex>
#include <algorithm>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class ConcurrentHashMap {
private:
    // Define the structure of each node in the hash map
    struct Node {
        Key key;
        Value value;
        Node(const Key& k, const Value& v) : key(k), value(v) {}
    };

    // Define the hash map buckets and associated mutexes
    std::vector<std::list<Node*>> buckets;
    std::vector<std::shared_mutex> mutexes;
    Hash hashFunction;

    // Get the mutex for a given key
    std::shared_mutex& getMutex(const Key& key) {
        std::size_t hashValue = hashFunction(key);
        return mutexes[hashValue % mutexes.size()];
    }

public:
    explicit ConcurrentHashMap(std::size_t num_buckets = 64) : buckets(num_buckets), mutexes(num_buckets) {}

    // Insert a key-value pair into the hash map
    void insert(const Key& key, const Value& value) {     
        std::size_t hashValue = hashFunction(key);
        std::size_t index = hashValue % buckets.size();
        Node* newNode = new Node(key, value);

        std::unique_lock lock(getMutex(key));
        buckets[index].push_back(newNode);
    }

    // Retrieve the value associated with a key from the hash map
    bool get(const Key& key, Value& value) {
        std::size_t hashValue = hashFunction(key);
        std::size_t index = hashValue % buckets.size();

        std::shared_lock lock(getMutex(key));
        auto const & bucket = buckets[index];
        auto iter = std::find_if(bucket.begin(), bucket.end(), [&key](const Node* node) {
            return node->key == key;
        });
        if (iter == bucket.end()) {
            return false;
        }
        value = (*iter)->value; 
        return true;
    }

    // Remove a key-value pair from the hash map
    void remove(const Key& key) {
        std::size_t hashValue = hashFunction(key);
        std::size_t index = hashValue % buckets.size();

        std::unique_lock lock(getMutex(key));
        auto & bucket = buckets[index];

        bucket.remove_if([&key](const Node* node) {
            return node->key == key;  
        });
    }

    // Print the contents of the hash map
    void print() {
       for (std::size_t i = 0; i < buckets.size(); ++i) {
            std::shared_lock lock(mutexes[i]);     
            auto & bucket = buckets[i];
            if (bucket.size()) {
                std::cout << "print bucket index[" << i << "]" << std::endl;
            }
            for (auto & node : bucket) {
                std::cout << "key: " << node->key << ", value: " << node->value << std::endl;
            }
       }
       std::cout << std::endl;
    }

    // Destructor to clean up memory
    ~ConcurrentHashMap() {
        for (std::size_t i = 0; i < buckets.size(); ++i) {
            std::unique_lock lock(mutexes[i]);
            auto & bucket = buckets[i];
            for (auto it = bucket.begin(); it != bucket.end();) {
                Node* del = *it;
                it = bucket.erase(it);
                delete del;
            }
        }

    }
};

int main()
{
    ConcurrentHashMap<std::string, int> hm(2);
    
    hm.insert(std::string("abcd"), 1);
    hm.insert(std::string("efgh"), 2);
    hm.insert(std::string("ihkk"), 3);
    hm.insert(std::string("jerry"), 4);
    hm.insert(std::string("lucy"), 5);
    hm.print();
    
    int value;
    std::string key="lucy";
    bool res = hm.get("lucy", value);
    if (res) {
        std::cout << "find value : "<< value << std::endl;
    } else {
        std::cout << "not find " << std::endl;
    }

    hm.remove("lucy");
    hm.print();

    return 0;
}

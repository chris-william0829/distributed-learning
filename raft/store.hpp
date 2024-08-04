#ifndef KVSTORE_STORE_HPP
#define KVSTORE_STORE_HPP

#include <unordered_map>
#include <string>
#include <shared_mutex>

class KVStore {
public:
    KVStore() = default;

    std::string Get(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(mu_);
        auto it = data_.find(key);
        return it != data_.end() ? it->second : "";
    }

    void Set(const std::string& key, const std::string& value) {
        std::unique_lock<std::shared_mutex> lock(mu_);
        data_[key] = value;
    }

    void Delete(const std::string& key) {
        std::unique_lock<std::shared_mutex> lock(mu_);
        data_.erase(key);
    }

private:
    std::unordered_map<std::string, std::string> data_;
    std::shared_mutex mu_;
};

#endif // KVSTORE_STORE_HPP

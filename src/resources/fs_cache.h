#pragma once

struct FsCache {
    static std::filesystem::path filepath_for_key(std::string key) {
        return std::filesystem::path("cache")
            / std::to_string(std::hash<std::string> {}(key));
    }

    static std::optional<std::ifstream> get(std::string key) {
        auto filepath = filepath_for_key(key);

        if (std::filesystem::exists(filepath)) {
            return std::ifstream(filepath, std::ios::binary);
        } else {
            return std::nullopt;
        }
    }

    template<class T>
    static void insert(std::string key, const std::vector<T>& data) {
        auto filepath = filepath_for_key(key);
        auto stream = std::ofstream(filepath, std::ios::binary);
        stream.write((char*)data.data(), data.size() * sizeof(T));
    }
};

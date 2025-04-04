#ifndef NGX_C_CONF_H
#define NGX_C_CONF_H

#include <cstdint>
#include <unordered_map>
#include <string>
#include <mutex>

namespace nginx {

    class CConfig {
    public:
        static CConfig &GetInstance() {
            static CConfig m_instance;
            return m_instance;
        }

        ~CConfig() = default;

        // 禁用拷贝和移动
        CConfig(const CConfig &other) = delete;

        CConfig &operator=(const CConfig &other) = delete;

        CConfig(const CConfig &&other) = delete;

        CConfig &operator=(const CConfig &&other) = delete;

        bool Load(const char *pConfName);

        const char *GetString(const char *pItemName);

        int32_t GetIntDefault(const char *pItemName);

    private:
        CConfig() = default;

    private:
        std::unordered_map<std::string, std::string> m_configItemMap{};
        std::mutex m_mutex{};
    };

} // namespace nginx

#endif // NGX_C_CONF_H

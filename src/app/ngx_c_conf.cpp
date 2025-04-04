#include "ngx_c_conf.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace nginx {

    bool CConfig::Load(const char *pConfName) {
        std::lock_guard<std::mutex> lock(m_mutex);
        int32_t fd = open(pConfName, O_RDONLY);
        if (fd == -1) {
            // todo
        }
        // todo
        close(fd);
        return false;
    }

    const char *CConfig::GetString(const char *pItemName) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto iter = m_configItemMap.find(pItemName);
        return (iter != m_configItemMap.end()) ? iter->second.c_str() : nullptr;
    }

    int32_t CConfig::GetIntDefault(const char *pItemName) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto iter = m_configItemMap.find(pItemName);
        return (iter != m_configItemMap.end()) ? std::stoi(iter->second) : 0;
    }

} // namespace nginx

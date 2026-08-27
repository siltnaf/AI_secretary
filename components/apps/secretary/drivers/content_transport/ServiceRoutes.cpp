#include <cstdio>
#include <cstring>

#include "ServiceRoutes.hpp"

namespace {

bool baseUrl(const char *content_url, char *buffer, size_t buffer_size)
{
    if (content_url == nullptr || buffer == nullptr || buffer_size == 0 ||
            (std::strncmp(content_url, "http://", 7) != 0 && std::strncmp(content_url, "https://", 8) != 0)) {
        return false;
    }

    std::strncpy(buffer, content_url, buffer_size - 1);
    buffer[buffer_size - 1] = '\0';
    char *query = std::strchr(buffer, '?');
    if (query != nullptr) {
        *query = '\0';
    }
    while (std::strlen(buffer) > 0 && buffer[std::strlen(buffer) - 1] == '/') {
        buffer[std::strlen(buffer) - 1] = '\0';
    }
    char *path = std::strstr(buffer + (std::strncmp(buffer, "https://", 8) == 0 ? 8 : 7), "/api/");
    if (path != nullptr) {
        *path = '\0';
    }
    return buffer[0] != '\0';
}

}

namespace SecretaryServiceRoutes {

bool isRemoteService(SecretaryPage page)
{
    return page == SecretaryPage::Library || page == SecretaryPage::Voice || page == SecretaryPage::Music ||
           page == SecretaryPage::Poem || page == SecretaryPage::Word || page == SecretaryPage::Cartoon ||
           page == SecretaryPage::Radio || page == SecretaryPage::FindHome || page == SecretaryPage::Chat ||
           page == SecretaryPage::Game;
}

bool build(SecretaryPage page, const char *content_url, char *buffer, size_t buffer_size)
{
    char base[256] = {};
    if (!baseUrl(content_url, base, sizeof(base))) {
        return false;
    }

    const char *path = nullptr;
    switch (page) {
    case SecretaryPage::Library: path = "/api/books"; break;
    case SecretaryPage::Voice: path = "/api/voice-stories"; break;
    case SecretaryPage::Music: path = "/api/kid-songs"; break;
    case SecretaryPage::Poem: path = "/api/poems"; break;
    case SecretaryPage::Word: path = "/api/words"; break;
    case SecretaryPage::Cartoon: path = "/api/cartoons"; break;
    case SecretaryPage::Radio: path = "/api/radio/list?limit=10&offset=0"; break;
    case SecretaryPage::FindHome: path = "/api/find-home/map.jpg?w=480&h=480&z=auto&simplified=1"; break;
    case SecretaryPage::Chat: path = "/api/chat/messages"; break;
    case SecretaryPage::Game: path = "/api/board/move"; break;
    default: return false;
    }
    return std::snprintf(buffer, buffer_size, "%s%s", base, path) > 0;
}

}
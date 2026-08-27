#include "PageCatalog.hpp"

const char *secretaryPageTitle(SecretaryPage page)
{
    switch (page) {
    case SecretaryPage::Home: return "Secretary";
    case SecretaryPage::Reader: return "Content Reader";
    case SecretaryPage::Library: return "Library";
    case SecretaryPage::Clock: return "Clock";
    case SecretaryPage::Calendar: return "Calendar";
    case SecretaryPage::Calculator: return "Calculator";
    case SecretaryPage::Music: return "Music";
    case SecretaryPage::Chat: return "Chat";
    case SecretaryPage::Game: return "Game";
    case SecretaryPage::Voice: return "Voice";
    case SecretaryPage::Recording: return "Recording";
    case SecretaryPage::Poem: return "Poems";
    case SecretaryPage::Word: return "Words";
    case SecretaryPage::Cartoon: return "Cartoons";
    case SecretaryPage::Radio: return "Radio";
    case SecretaryPage::FindHome: return "Find Home";
    case SecretaryPage::Settings: return "Settings";
    }
    return "Secretary";
}
#pragma once

enum class SecretaryPage {
    Home,
    Reader,
    Library,
    Clock,
    Calendar,
    Calculator,
    Music,
    Chat,
    Game,
    Voice,
    Recording,
    Poem,
    Word,
    Cartoon,
    Radio,
    FindHome,
    Settings,
};

const char *secretaryPageTitle(SecretaryPage page);
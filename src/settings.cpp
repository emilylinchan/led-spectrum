/*
    spectrum - A real-time command line audio visualizer
    Copyright (C) 2026 Joe R. (@johnmilson125-png)
    Copyright (C) 2026 Majock Bim (@majockbim)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "../inc/settings/settings.hpp"

#define MAX_DATA_SIZE 1024

template <typename T> 
constexpr inline size_t __cdecl ToIndex(T index) noexcept 
{
    if (index > 0) {return(size_t(index));} return(size_t(0));
}

void __cdecl ClearConsole(void) 
{
    HANDLE StdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    CONSOLE_SCREEN_BUFFER_INFO csbi{};
    GetConsoleScreenBufferInfo(StdOutput, &csbi);
    FillConsoleOutputCharacter(StdOutput, ' ', csbi.dwSize.X * csbi.dwSize.Y, {0, 0}, &written);
    FillConsoleOutputAttribute(StdOutput, csbi.wAttributes, csbi.dwSize.X * csbi.dwSize.Y, {0, 0}, &written);
    SetConsoleCursorPosition(StdOutput, {0, 0});
}

int __cdecl ShowOption(_In_ struct Option* option) 
{
    if (option) {
        std::cout << "\033[48;2;" << std::to_string(option->backgroundColor.r) + ";" + std::to_string(option->backgroundColor.g) + ";" + std::to_string(option->backgroundColor.b) + "m";
        std::cout << "\033[38;2;" << std::to_string(option->foregroundColor.r) + ";" + std::to_string(option->foregroundColor.g) + ";" + std::to_string(option->foregroundColor.b) + "m" << option->prefix << " ";
        std::cout << "\033[97m" << option->text;
        std::cout << "\033[48;2;" << std::to_string(option->backgroundColor.r) + ";" + std::to_string(option->backgroundColor.g) + ";" + std::to_string(option->backgroundColor.b) + "m";
        std::cout << "\033[38;2;" << std::to_string(option->foregroundColor.r) + ";" + std::to_string(option->foregroundColor.g) + ";" + std::to_string(option->foregroundColor.b) + "m " << option->prefix << std::endl;
        std::cout << "\033[0m";
    } else {
        return(-1);
    }
    return(0);
}

int __cdecl JsonFileFinder::FindJsonFiles(_In_ JsonFileReader* fileReader) {
    struct _finddata_t jsonFilesFinder;
    intptr_t handle;
    std::vector<struct Theme> themes;

    // 1. Inject the default Pink Theme at index 0
    struct Theme pinkTheme{};
    strcpy_s(pinkTheme.themeName, "Pink Theme");
    strcpy_s(pinkTheme.themeId, "pink-default");
    strcpy_s(pinkTheme.themeMode, "Pink Mode");
    pinkTheme.colorRed = 1.0;
    pinkTheme.colorGreen = 0.41; 
    pinkTheme.colorBlue = 0.70;  
    pinkTheme.key = 49; // Key '1'
    themes.push_back(pinkTheme);

    // 2. Inject the default Gradient Theme at index 1
    struct Theme gradientTheme{};
    strcpy_s(gradientTheme.themeName, "Gradient Theme");
    strcpy_s(gradientTheme.themeId, "gradient-default");
    strcpy_s(gradientTheme.themeMode, "Gradient Mode");
    gradientTheme.colorRed = 1.0;
    gradientTheme.colorGreen = 1.0; 
    gradientTheme.colorBlue = 1.0;  
    gradientTheme.key = 50; // Key '2'
    themes.push_back(gradientTheme);

    // 3. Scan for other themes
    handle = _findfirst("themes\\*.json", &jsonFilesFinder);
    if (handle != -1L) {
        do {
            struct Theme oneTheme{};
            FilePtr jsonFile;
            
            char fullThemesPath[MAX_PATH];
            sprintf_s(fullThemesPath, MAX_PATH, "themes\\%s", jsonFilesFinder.name);

            fopen_s(&jsonFile, fullThemesPath, "r");
            if (!jsonFile) {
                continue;
            } 

            struct JsonValue JsonThemeOptions[] = {
                {VT_Double, {.doubleValue = 1.0}, nullptr, &oneTheme.colorRed, "spectrum.tui.colorR", "theme.properties"},
                {VT_Double, {.doubleValue = 1.0}, nullptr, &oneTheme.colorGreen, "spectrum.tui.colorG", "theme.properties"},
                {VT_Double, {.doubleValue = 1.0}, nullptr, &oneTheme.colorBlue, "spectrum.tui.colorB", "theme.properties"},
                {VT_Boolean, {.booleanValue = false}, nullptr, &oneTheme.useRandomCharacters, "spectrum.tui.useRandomCharacters", "theme.properties"},
                {VT_Character, {.characterValue = '='}, nullptr, &oneTheme.customCharacter, "spectrum.tui.customCharacter", "theme.properties"},
                {VT_String, {.stringValue = "Default Mode"}, nullptr, &oneTheme.themeMode, "spectrum.tui.visualizerMode", "theme.properties"},
                {VT_Int, {.intValue = 0}, nullptr, &oneTheme.key, "spectrum.tui.key", "theme.properties"},
                {VT_String, {.stringValue = "**Untitled-Theme**"}, nullptr, &oneTheme.themeName, "theme.name", "root"},
                {VT_String, {.stringValue = "**No-ID**"}, nullptr, &oneTheme.themeId, "theme.id", "root"}
            };

            if (fileReader->ReadSettings(jsonFile, JsonThemeOptions, sizeof(JsonThemeOptions) / sizeof(struct JsonValue)) == 0) {
                if (oneTheme.key == 49 || oneTheme.key == 50) {
                    oneTheme.key = 0; // Prevent overriding Pink Theme or Gradient Theme hotkeys
                }
                themes.push_back(oneTheme);
            }
            if (jsonFile) fclose(jsonFile);
        } while (_findnext(handle, &jsonFilesFinder) == 0);
        _findclose(handle);
    }

    // 4. Load global settings (quietly)
    struct JsonValue JsonConfigs[] = {
        {VT_Boolean, {.booleanValue = false}, nullptr, &fileReader->showMenu, "spectrum.tui.showMenu", "root"},
        {VT_Boolean, {.booleanValue = true}, nullptr, &fileReader->noBgColor, "spectrum.tui.noBackgroundColor", "root"}
    };

    FilePtr configs;
    fopen_s(&configs, "settings.json", "r");
    if (configs) {
        fileReader->ReadSettings(configs, JsonConfigs, sizeof(JsonConfigs) / sizeof(struct JsonValue));
        fclose(configs);
    }

    // 5. Set Pink Theme as current and exit
    fileReader->currentTheme = themes.at(0);
    fileReader->themes = themes;

    return(0);
}

int __cdecl JsonFileReader::ReadSettings(_In_ FilePtr jsonFile, _In_ struct JsonValue* jsonThemeOptions, size_t _Size) {
    if (!jsonThemeOptions) return(-1);
    if (!jsonFile) return(-19);

    char data[MAX_DATA_SIZE];
    std::string text;
    while (fgets(data, MAX_DATA_SIZE, jsonFile)) {
        if (strlen(data) > 0 && data[ToIndex<int>(strlen(data) - 1)] == '\n') {
            data[ToIndex<int>(strlen(data) - 1)] = ' ';
        }
        text += data;
    }
    yyjson_doc *doc = yyjson_read(text.c_str(), text.length(), 0);
    if (!doc) {
        return(-1);
    }
    yyjson_val *root = yyjson_doc_get_root(doc);
    if (!root) {
        yyjson_doc_free(doc);
        return(-2);
    }

    size_t index = 0;
    while (index < _Size) {
        yyjson_val* currentRoot;
        if (strcmp(jsonThemeOptions[index].root, "root") != 0) {
            currentRoot = yyjson_obj_get(root, jsonThemeOptions[index].root);
        } else {
            currentRoot = root;
        }

        if (currentRoot) {
            jsonThemeOptions[index].pointer = yyjson_obj_get(currentRoot, jsonThemeOptions[index].name);
        } else {
            jsonThemeOptions[index].pointer = nullptr;
        }

        if (jsonThemeOptions[index].pointer) {
            switch (jsonThemeOptions[index].valueType) 
            {
                case VT_Double: {
                    CopyValue<double>(jsonThemeOptions[index].defaultValue.doubleValue, yyjson_get_num(jsonThemeOptions[index].pointer), jsonThemeOptions[index].pointer1);
                } break;

                case VT_Boolean: {
                    CopyValue<bool>(jsonThemeOptions[index].defaultValue.booleanValue, yyjson_get_bool(jsonThemeOptions[index].pointer), jsonThemeOptions[index].pointer1);
                } break;

                case VT_Int: {
                    CopyValue<int>(jsonThemeOptions[index].defaultValue.intValue, (int)yyjson_get_int(jsonThemeOptions[index].pointer), jsonThemeOptions[index].pointer1);
                } break;

                case VT_Character: {
                    const char* tmpString = yyjson_get_str(jsonThemeOptions[index].pointer);
                    if (tmpString) {
                        CopyValue<char>(jsonThemeOptions[index].defaultValue.characterValue, tmpString[0], jsonThemeOptions[index].pointer1);
                    } else {
                         CopyValue<char>(jsonThemeOptions[index].defaultValue.characterValue, jsonThemeOptions[index].defaultValue.characterValue, jsonThemeOptions[index].pointer1);
                    }
                } break;

                case VT_String: {
                    const char* tmpString = yyjson_get_str(jsonThemeOptions[index].pointer);
                    char* _Ptr = reinterpret_cast<char*>(jsonThemeOptions[index].pointer1);
                    if (_Ptr) {
                        if (tmpString) {
                            strcpy_s(_Ptr, MAX_THEME_NAME, tmpString);
                        } else if (jsonThemeOptions[index].defaultValue.stringValue) {
                            strcpy_s(_Ptr, MAX_THEME_NAME, jsonThemeOptions[index].defaultValue.stringValue);
                        }
                    }
                } break;
            }
        } else {
             switch (jsonThemeOptions[index].valueType) 
            {
                case VT_Double: {
                    CopyValue<double>(jsonThemeOptions[index].defaultValue.doubleValue, jsonThemeOptions[index].defaultValue.doubleValue, jsonThemeOptions[index].pointer1);
                } break;

                case VT_Boolean: {
                    CopyValue<bool>(jsonThemeOptions[index].defaultValue.booleanValue, jsonThemeOptions[index].defaultValue.booleanValue, jsonThemeOptions[index].pointer1);
                } break;

                case VT_Int: {
                    CopyValue<int>(jsonThemeOptions[index].defaultValue.intValue, jsonThemeOptions[index].defaultValue.intValue, jsonThemeOptions[index].pointer1);
                } break;

                case VT_Character: {
                    CopyValue<char>(jsonThemeOptions[index].defaultValue.characterValue, jsonThemeOptions[index].defaultValue.characterValue, jsonThemeOptions[index].pointer1);
                } break;

                case VT_String: {
                    char* _Ptr = reinterpret_cast<char*>(jsonThemeOptions[index].pointer1);
                    if (_Ptr && jsonThemeOptions[index].defaultValue.stringValue) {
                        strcpy_s(_Ptr, MAX_THEME_NAME, jsonThemeOptions[index].defaultValue.stringValue);
                    }
                } break;
            }
        }
        index++;
    }

    yyjson_doc_free(doc);
    return(0);
}

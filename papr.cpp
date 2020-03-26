#include<string.h>
#include<string>

#include<fstream>
#include<iostream>
#include<filesystem>
#include<readline/readline.h>

#include "yaml-cpp/yaml.h"

namespace fs = std::filesystem;

using std::cout;
using std::endl;

static const char USAGE[] =
R"(Papr.

    Usage:                  papr [-h | -v | -i | OPTION...]
      Add a wallpaper:      papr add [path]
      Rename a wallpaper:   papr rename [path] [new file name]
      Print artist info:    papr info [-nuw | -a]
      Print some stats:     papr stats

    Options: 
      -h --help             Show this screen.
      -v --version          Show version.
      -i --init             Initializes config file.
      -n --name             Print artist name.
      -u --url              Print artist url. DEFAULT
      -w --works            Print artist wallpaper path(s).
      -a --all              Print all artist info.
)";

#define VERSION "v0.1"
#define FILE ".papr"

#define KEY_FOLDER "folder"
#define KEY_ARTISTS "artists"
#define KEY_NAME "name"
#define KEY_LINK "link"
#define KEY_WORKS "works"
#define UNKNOWN "Unknown"
#define NONE "N/A"

void showHelp();
void showVersion();
void init();

int add(char *);
std::string input(const char *, const char * = nullptr);
YAML::Node newArtist(std::string name, std::string path);

// I'm not sure how to use a single autocomplete function, so copy+paste is how it'll be
static const char **autocomplete_artists;
char **artist_completion(const char *, int, int);
char *artist_generator(const char *, int);
static const char **autocomplete_categories;
char **category_completion(const char *, int, int);
char *category_generator(const char *, int);

char *strdup(const char *);
const char *getExtension(char *);
bool changeFileName(std::string &);
std::string &trim(std::string &);

void stats();

///
/// Papr is a wallpaper manager that specializes
///  in categorizing and storing wallpapers.
///
/// Settings file contains artists with wallpaper paths
///  -> .papr
///
/// C++17 -> https://en.cppreference.com/w/cpp/filesystem
///
int main(int argc, char** argv) {

    if(argc == 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        showHelp();
        return 0;
    } else if(strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0) {
        cout << "Papr " << VERSION << endl;
    } else if(strcmp(argv[1], "-i") == 0 || strcmp(argv[1], "--init") == 0) {
        init();
    } else if(strcmp(argv[1], "add") == 0) {
        if(argc > 2) {
            if(strcmp(argv[2], "artist") == 0 || strcmp(argv[2], "-a") == 0) {
                // add artist page to txt
                cout << "Not done." << endl;
            } else {
                return add(argv[2]);
            }
        } else {
            cout << "Path required to file being added!" << endl;
        }
    } else if(strcmp(argv[1], "stats") == 0) {
        stats();
    } else {
        cout << "Use \"papr -h\" for help." << endl;
    }

    return 0;
}

void showHelp() {
    cout << USAGE << endl;
}

void init() {
    if(fs::exists(FILE)) {
        cout << "File " << FILE << " already exists. Aborting." << endl;
        return;
    }

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << KEY_FOLDER << YAML::Value << "wallpapers";
    out << YAML::Key << KEY_ARTISTS;
    out << YAML::Value << YAML::BeginSeq << YAML::BeginMap;
    out << YAML::Key << KEY_NAME << YAML::Value << UNKNOWN;
    out << YAML::Key << KEY_LINK << YAML::Value << NONE;
    out << YAML::Key << KEY_WORKS << YAML::Value << YAML::BeginSeq << YAML::EndSeq;
    out << YAML::EndMap << YAML::EndSeq << YAML::EndMap;
    
    if(!fs::exists("wallpapers"))
        fs::create_directory("wallpapers");

    std::ofstream fout(FILE);
    fout << out.c_str();
    fout.close();
}

int add(char *path) {
    // load config file
    YAML::Node config = YAML::LoadFile(FILE);

    // load artists from config
    YAML::Node artistConfig = config[KEY_ARTISTS];
    std::vector<const char *> artists;
    for(int i = 0; i < artistConfig.size(); i++)
        artists.push_back(artistConfig[i][KEY_NAME].as<std::string>().c_str());
    artists.push_back(NULL);
    
    autocomplete_artists = &artists[0];

    // load all categories
    std::vector<const char *> categories;
    for(auto& p : fs::directory_iterator())
        if(p.is_directory())
            categories.push_back(p.path().filename().c_str());
    categories.push_back(NULL);
        
    autocomplete_categories = &categories[0];

    // &vec[0] converts to array automatically because spec guarantees element storage is contiguous
    rl_attempted_completion_function = artist_completion;
    std::string artist = input("Artist: ", UNKNOWN);
    rl_attempted_completion_function = NULL;
    std::string fileName = input("New File Name (description): ");
    
    if(changeFileName(fileName)) {
        cout << "File name changed: " << fileName << endl;
    }
    
    rl_attempted_completion_function = category_completion;
    std::string category = input("Category: ", "Misc");

    std::string newPath = fileName + getExtension(path);

    // rename and move "path" to "wallpaper folder/category/fileName"
    fs::path categoryFolder = fs::current_path() / config[KEY_FOLDER].as<std::string>() / category;
    fs::path from = fs::current_path() / path;
    fs::path to = categoryFolder / newPath;
    std::error_code err;
    
    fs::create_directories(categoryFolder); // create directories if they don't exist already
    fs::rename(from, to, err); // move and rename
    if(err) {
        std::cerr << err.message() << endl;
        return 1;
    }

    // add "category/fileName" to artist yaml
    bool added = false;
    for(int i = 0; i < artistConfig.size(); i++) {
        if(artistConfig[i][KEY_NAME].as<std::string>() == artist) {
            artistConfig[i][KEY_WORKS].push_back(newPath);
            added = true;
            break;
        }
    }
    
    if(!added) {
        artistConfig.push_back(newArtist(artist, newPath));
    }

    // put back in config file
    std::ofstream fout(FILE);
    fout << config;
    fout.close();

    return 0;
}

YAML::Node newArtist(std::string name, std::string path) {
    // ask for website link
    cout << name << " seems to be a new artist. Please give a link to where the wallpaper came from." << endl;
    std::string link = input("Link (leave blank for none): ", NONE);
    
    YAML::Node node;
    node[KEY_NAME] = name;
    node[KEY_LINK] = link;
    node[KEY_WORKS].push_back(path);
    
    return node;
}

std::string input(const char *prompt, const char *defaultInput) {

    // GNU Readline is very nice
    do {
        char *buffer = readline(prompt);
        if(buffer && *buffer) {
            std::string rtn = buffer;
            trim(rtn);
            free(buffer);
            return rtn;
        } else if(!defaultInput) {
            cout << "You need to have a file name!" << endl;
        }
    } while(!defaultInput);

    return defaultInput;
}

char **artist_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, artist_generator);
}

char *artist_generator(const char *text, int state) {
    static int i, len;
    const char *str;

    if(!state) {
        i = 0;
        len = strlen(text);
    }

    while(str = autocomplete_artists[i++]) {
        if(strncmp(str, text, len) == 0) {
            return strdup(str);
        }
    }

    return NULL;
}

char **category_completion(const char *text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, category_generator);
}

char *category_generator(const char *text, int state) {
    static int i, len;
    const char *str;

    if(!state) {
        i = 0;
        len = strlen(text);
    }

    while(str = autocomplete_categories[i++]) {
        if(strncmp(str, text, len) == 0) {
            return strdup(str);
        }
    }

    return NULL;
}

// Not a standard c function, so here it is. Taken from the web.
char *strdup(const char *s) {
    size_t slen = strlen(s);
    char* result = (char *) malloc(slen + 1);
    if(result == NULL) {
        return NULL;
    }

    memcpy(result, s, slen+1);
    return result;
}

const char *getExtension(char *path) {
    for(int i = strlen(path) - 1; i >= 0; i--)
        if(path[i] == '.')
            return path + i;
    return "";
}

bool changeFileName(std::string &str) {
    // POSIX "Fully portable filenames" allowed characters + space: A-Z a-z 0-9 . _ - (and space)
    // remove all characters that are not allowed and make sure a hypen is not at the front
    bool changed = false;
    for(int i = 0; i < str.length(); i++) {
        char c = str[i];
        if((c < 'A' || c > 'Z') 
            && (c < 'a' || c > 'z') 
            && (c < '0' || c > '9')
            && (c != '.' || c != '_' || c != '-' || c != ' ') 
            || i == 0 && c == '-') {
            str.erase(i--, 1);
            changed = true;
        }
    }
    
    return changed;
}

std::string &trim(std::string &str) {
    // remove all spaces to the left
    int i;
    for(i = 0; i < str.length() && str[i] == ' '; i++);
    // index i is the first index that is not a space
    str.erase(0, i);

    // remove all spaces to the right
    for(i = str.length() - 1; i >= 0 && str[i] == ' '; i--);
    // index i is the last index that is not a space
    str.erase(i + 1, str.length() - i);

    return str;
}

void stats() {
    // total size, total number, number of pixels?
    cout << "Not done." << endl;
}
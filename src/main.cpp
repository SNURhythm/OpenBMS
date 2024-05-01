#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <unordered_set>

#include "main.h" // doesn't work

#include <filesystem>
#ifdef _WIN32
#include <windows.h>

#elif __APPLE__
#include "TargetConditionals.h"
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
// define something for simulator
#elif TARGET_OS_IPHONE
// define something for iphone
#else
#define TARGET_OS_OSX 1
// define something for OSX
#include <dirent.h>
#include <sys/stat.h>
#endif
#elif __linux
// linux
#include <dirent.h>
#include <sys/stat.h>
#elif __unix // all unices not caught above
// Unix
#elif __posix
// POSIX
#endif
#include <SDL2/SDL.h>
class MainFunctionRAII
{
private:
    std::atomic<bool> &quitFlag;

public:
    MainFunctionRAII(std::atomic<bool> &flag) : quitFlag(flag)
    {
        quitFlag = false;
    }

    ~MainFunctionRAII()
    {
        quitFlag = true;
        std::cout << "Main function is quitting..." << std::endl;
    }
};
int main(int argv, char **args)
{
    std::atomic<bool> quitFlag(false);
    
#ifdef TARGET_OS_OSX
    const char *runpath = args[0];
    std::string plugin_path = runpath;
    plugin_path = plugin_path.substr(0, plugin_path.find_last_of("/"));
    plugin_path += "/plugins";
    setenv("VLC_PLUGIN_PATH", plugin_path.c_str(), 1);
    std::cout << "VLC_PLUGIN_PATH: " << getenv("VLC_PLUGIN_PATH") << std::endl;
#endif
    using std::cerr;
    using std::endl;
    auto dbHelper = ChartDBHelper::GetInstance();
    auto db = dbHelper.Connect();
    dbHelper.CreateChartMetaTable(db);
    dbHelper.CreateEntriesTable(db);
    auto entries = dbHelper.SelectAllEntries(db);
    // open folder select if no entries
    if (entries.size() == 0)
    {
        const char *folder = tinyfd_selectFolderDialog("Select Folder", "");
        if (folder == nullptr)
        {
            return EXIT_FAILURE;
        }
        std::filesystem::path path(folder);
        dbHelper.InsertEntry(db, path);
    }
    std::thread t1(LoadCharts, std::ref(dbHelper), std::ref(db), std::ref(quitFlag));
    threadRAII t1_raii(std::move(t1));
    MainFunctionRAII mainFunctionRAII(quitFlag);
    // vlc instance
    std::cout << "VLC init..." << std::endl;
    auto instance = VLC::Instance(0, nullptr);

    std::cout << "VLC init done" << std::endl;
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
    {
        cerr << "SDL_Init Error: " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    SDL_Window *win = SDL_CreateWindow("Hello World!", 100, 100, 620, 387, SDL_WINDOW_SHOWN);
    if (win == nullptr)
    {
        cerr << "SDL_CreateWindow Error: " << SDL_GetError() << endl;
        return EXIT_FAILURE;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr)
    {
        cerr << "SDL_CreateRenderer Error" << SDL_GetError() << endl;
        if (win != nullptr)
        {
            SDL_DestroyWindow(win);
        }
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Surface *bmp = SDL_LoadBMP("./res/img/sdl.bmp");
    if (bmp == nullptr)
    {
        cerr << "SDL_LoadBMP Error: " << SDL_GetError() << endl;
        if (ren != nullptr)
        {
            SDL_DestroyRenderer(ren);
        }

        if (win != nullptr)
        {
            SDL_DestroyWindow(win);
        }
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, bmp);

    if (tex == nullptr)
    {
        cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << endl;
        if (bmp != nullptr)
        {
            SDL_FreeSurface(bmp);
        }
        if (ren != nullptr)
        {
            SDL_DestroyRenderer(ren);
        }
        if (win != nullptr)
        {
            SDL_DestroyWindow(win);
        }
        SDL_Quit();
        return EXIT_FAILURE;
    }
    SDL_FreeSurface(bmp);

    SDL_RenderClear(ren);
    SDL_RenderCopy(ren, tex, nullptr, nullptr);
    SDL_RenderPresent(ren);

    SDL_Event e;
    bool quit = false;
    while (!quit)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN)
            {
                quit = true;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN)
            {
                quit = true;
            }
        }
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    std::cout << "SDL quit" << std::endl;

    return EXIT_SUCCESS;
}

void LoadCharts(ChartDBHelper &dbHelper, sqlite3 *db, std::atomic_bool &isCancelled)
{
    std::vector<bms_parser::ChartMeta> chartMetas;
    dbHelper.SelectAllChartMeta(db, chartMetas);
    // sort by title
    std::sort(chartMetas.begin(), chartMetas.end(), [](bms_parser::ChartMeta &a, bms_parser::ChartMeta &b)
              { return a.Title < b.Title; });
    std::vector<Diff> diffs;
    std::cout << "Finding new bms files" << std::endl;
    auto entries = dbHelper.SelectAllEntries(db);
    std::unordered_set<std::wstring> oldFilesWs;

    for (auto &chartMeta : chartMetas)
    {
        if (isCancelled)
        {
            break;
        }
        oldFilesWs.insert(chartMeta.BmsPath);
    }
    for (auto &entry : entries)
    {
        if (isCancelled)
        {
            break;
        }
        FindNewBmsFiles(diffs, oldFilesWs, entry, isCancelled);
    }

    std::cout << "Found " << diffs.size() << " new bms files" << std::endl;
    std::atomic_bool is_committing(false);
    std::atomic_int success_count(0);
    dbHelper.BeginTransaction(db);
    parallel_for(diffs.size(), [&](int start, int end)
                 {
        for (int i = start; i < end; i++)
        {
            if (isCancelled)
            {
                break;
            }
            auto &diff = diffs[i];
            if (diff.type == Added)
            {
                bms_parser::Parser parser;
                bms_parser::Chart *chart;
                std::atomic_bool cancel(false);
                bms_parser::ChartMeta chartMeta;
                try{
                    parser.Parse(diffs[i].path.wstring(), &chart, false, true, cancel);
                } catch (std::exception &e){
                    delete chart;
                    std::cerr << "Error parsing " << diffs[i].path << ": " << e.what() << std::endl;
                    continue;
                }

                if (chart == nullptr) continue;
                ++success_count;
                if(success_count % 1000 == 0 && !is_committing){
                    is_committing = true;
                    dbHelper.CommitTransaction(db);
                    dbHelper.BeginTransaction(db);
                    is_committing = false;
                }
                dbHelper.InsertChartMeta(db, chart->Meta);
                delete chart;
            }
        } });
    dbHelper.CommitTransaction(db);
    std::cout << "Inserted " << success_count << " new charts" << std::endl;
}

#ifdef _WIN32
void FindFilesWin(const std::filesystem::path &path, std::vector<Diff> &diffs, const std::unordered_set<std::wstring> &oldFilesWs, std::vector<std::wstring> &directoriesToVisit, std::atomic_bool &isCancelled)
{
    WIN32_FIND_DATAW findFileData;
    HANDLE hFind = FindFirstFileW((path.wstring() + L"\\*.*").c_str(), &findFileData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (isCancelled)
            {
                FindClose(hFind);
                break;
            }
            if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                std::wstring filename(findFileData.cFileName);

                if (filename.size() > 4)
                {
                    std::wstring ext = filename.substr(filename.size() - 4);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
                    if (ext == L".bms" || ext == L".bme" || ext == L".bml")
                    {
                        std::wstring dirPath;

                        std::wstring fullPath = path.wstring() + L"\\" + filename;
                        if (oldFilesWs.find(fullPath) == oldFilesWs.end())
                        {
                            diffs.push_back({fullPath, Added});
                        }
                    }
                }
            }
            else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                std::wstring filename(findFileData.cFileName);

                if (filename != L"." && filename != L"..")
                {
                    directoriesToVisit.push_back(path.wstring() + L"\\" + filename);
                }
            }
        } while (FindNextFileW(hFind, &findFileData) != 0);
        FindClose(hFind);
    }
}
#else
void resolveDType(const std::filesystem::path &directoryPath, struct dirent *entry)
{
    if (entry->d_type == DT_UNKNOWN)
    {
        std::filesystem::path fullPath = directoryPath / entry->d_name;
        struct stat statbuf;
        if (stat(fullPath.c_str(), &statbuf) == 0)
        {
            if (S_ISREG(statbuf.st_mode))
            {
                entry->d_type = DT_REG;
            }
            else if (S_ISDIR(statbuf.st_mode))
            {
                entry->d_type = DT_DIR;
            }
        }
    }
}
// TODO: Use platform-specific method for faster traversal
void FindFilesUnix(const std::filesystem::path &directoryPath, std::vector<Diff> &diffs, const std::unordered_set<std::wstring> &oldFiles, std::vector<std::filesystem::path> &directoriesToVisit, std::atomic_bool &isCancelled)
{
    DIR *dir = opendir(directoryPath.c_str());
    if (dir)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            if (isCancelled)
            {
                closedir(dir);
                break;
            }
            resolveDType(directoryPath, entry);
            if (entry->d_type == DT_REG)
            {
                std::string filename = entry->d_name;
                if (filename.size() > 4)
                {
                    std::string ext = filename.substr(filename.size() - 4);
                    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                    if (ext == ".bms" || ext == ".bme" || ext == ".bml")
                    {
                        std::filesystem::path fullPath = directoryPath / filename;
                        if (oldFiles.find(fullPath.wstring()) == oldFiles.end())
                        {
                            diffs.push_back({fullPath, Added});
                        }
                    }
                }
            }
            else if (entry->d_type == DT_DIR)
            {
                std::string filename = entry->d_name;
                if (filename != "." && filename != "..")
                {
                    directoriesToVisit.push_back(directoryPath / filename);
                }
            }
        }
        closedir(dir);
    }
}
#endif

void FindNewBmsFiles(std::vector<Diff> &diffs, const std::unordered_set<std::wstring> &oldFilesWs, const std::filesystem::path &path, std::atomic_bool &isCancelled)
{
#ifdef _WIN32
    std::vector<std::wstring> directoriesToVisit;
    directoriesToVisit.push_back(path.wstring());
#else
    std::vector<std::filesystem::path> directoriesToVisit;
    directoriesToVisit.push_back(path);
#endif

    while (!directoriesToVisit.empty())
    {
        if (isCancelled)
        {
            break;
        }
        std::filesystem::path currentDir = directoriesToVisit.back();
        directoriesToVisit.pop_back();

#ifdef _WIN32

        FindFilesWin(currentDir, diffs, oldFilesWs, directoriesToVisit, isCancelled);
#else
        FindFilesUnix(currentDir, diffs, oldFilesWs, directoriesToVisit, isCancelled);
#endif
    }
}
#include "MainMenuScene.h"
#include "../tinyfiledialogs.h"
#include <fstream>
#include "../view/ChartListItemView.h"
#include "../view/TextView.h"
#include "../view/TextInputBox.h"
#include "../Utils.h"
#include "../targets.h"
#include "../video/transcode.h"
#include "../view/Button.h"
#include "GamePlayScene.h"
#ifdef _WIN32
#include <windows.h>

#elif __APPLE__

#include "TargetConditionals.h"
#if TARGET_OS_IPHONE && TARGET_IPHONE_SIMULATOR
// define something for simulator
#elif TARGET_OS_IPHONE
#include "../iOSNatives.hpp"
// define something for iphone
#include <dirent.h>
#include <sys/stat.h>
#else
// define something for OSX
#include "../MacNatives.h"
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
#include <iostream>

void MainMenuScene::init() {
  // Initialize the scene
  db = ChartDBHelper::GetInstance().Connect();
  initView(context);
  SDL_Log("Main Menu Scene Initialized");
  checkEntriesThread =
      std::thread(CheckEntries, std::ref(context), std::ref(*this));
}

void MainMenuScene::CheckEntries(ApplicationContext &context,
                                 MainMenuScene &scene) {
  auto dbHelper = ChartDBHelper::GetInstance();
  auto db = dbHelper.Connect();
  dbHelper.CreateChartMetaTable(db);
  dbHelper.CreateEntriesTable(db);
  auto entries = dbHelper.SelectAllEntries(db);
  // open folder select if no entries
  if (entries.empty()) {
#if TARGET_OS_IOS
    auto path = Utils::GetDocumentsPath("BMS");
    std::filesystem::create_directories(path);
    entries.push_back(path);
    // for iOS, not writing the entry to db is intentional, since it may change
    // due to update with new code signing
#else
    char *folder_c = tinyfd_selectFolderDialog("Select Folder", nullptr);
    std::string folder;
    if (folder_c == nullptr) {
      std::cerr << "tinyfd_selectFolderDialog error: " << strerror(errno)
                << std::endl;
      // get input from stdin
      std::cout << "Failed to open folder select dialog.\n";
      while (folder.empty()) {
        std::cout << "Enter bms folder path: ";
        std::cin >> folder;
        if (std::cin.eof() || std::cin.fail()) {
          break;
        }
        if (folder.empty()) {
          continue;
        }

        // replace ~ with home directory
        if (folder[0] == '~') {
          folder.replace(0, 1, getenv("HOME"));
        }
        std::ifstream test(folder);
        if (!test)
          folder = "";
      }
    } else {
      folder = folder_c;
    }
    std::filesystem::path path(folder);
    dbHelper.InsertEntry(db, path);
#endif
  }
  LoadCharts(dbHelper, db, entries, scene, context.quitFlag);
  dbHelper.Close(db);
}

void MainMenuScene::initView(ApplicationContext &context) {
  // Initialize the view

  recyclerView = new RecyclerView<bms_parser::ChartMeta>(
      0, 0, rendering::window_width, rendering::window_height,
      [](const bms_parser::ChartMeta &a, const bms_parser::ChartMeta &b) {
        return a.SHA256 == b.SHA256;
      });
  recyclerView->onCreateView = [this](const bms_parser::ChartMeta &item) {
    return new ChartListItemView(0, 0, rendering::window_width, 100, item.Title,
                                 item.Artist, std::to_string(item.PlayLevel));
  };
  recyclerView->itemHeight = 100;
  recyclerView->onBind = [this](View *view, const bms_parser::ChartMeta &item,
                                int idx, bool isSelected) {
    auto *chartListItemView = dynamic_cast<ChartListItemView *>(view);
    chartListItemView->setTitle(item.Title);
    chartListItemView->setArtist(item.Artist);
    chartListItemView->setLevel(std::to_string(item.PlayLevel));
    if (!item.Banner.empty())
      chartListItemView->setBanner(item.Folder / item.Banner);
    else
      chartListItemView->unsetBanner();
    if (isSelected) {
      chartListItemView->onSelected();
    } else {
      chartListItemView->onUnselected();
    }
  };

  auto jacketView = new ImageView(0, 0, 0, 0);
  recyclerView->onSelected = [this, &context, jacketView](
                                 const bms_parser::ChartMeta &item, int idx) {
    if (willStart)
      return;
    auto selectedView = recyclerView->getViewByIndex(idx);
    SDL_Log("Selected: %s", item.Title.c_str());
    if (selectedView) {
      selectedView->onSelected();
    }
    jacketView->setImage(item.Folder / item.StageFile);
    previewLoadCancelled = true;
    if (loadThread.joinable()) {
      SDL_Log("Joining preview thread");
      loadThread.join();
    }
    if (selectedChart) {
      delete selectedChart;
      selectedChart = nullptr;
    }
    loadThread = std::thread([this, item, &context]() {
      SDL_Log("Previewing %s", path_t_to_utf8(item.BmsPath).c_str());
      context.jukebox.stop();
      previewLoadCancelled = false;
      bms_parser::Parser parser;
      bms_parser::Chart *chart;

      try {
        SDL_Log("Parsing %s", path_t_to_utf8(item.BmsPath).c_str());
        parser.Parse(item.BmsPath, &chart, false, false, previewLoadCancelled);
        SDL_Log("Parsed %s", path_t_to_utf8(item.BmsPath).c_str());
      } catch (std::exception &e) {
        delete chart;
        SDL_Log("Error parsing %s: %s", path_t_to_utf8(item.BmsPath).c_str(),
                e.what());
        return;
      }
      selectedChart = chart;

      context.jukebox.loadChart(*chart, previewLoadCancelled);
      if (!willStart) {
        context.jukebox.play();
      }
    });
  };
  recyclerView->onUnselected = [this](const bms_parser::ChartMeta &item,
                                      int idx) {
    auto unselectedView = recyclerView->getViewByIndex(idx);
    if (unselectedView) {
      unselectedView->onUnselected();
    }
  };

  rootLayout =
      new LinearLayout(0, 0, rendering::window_width, rendering::window_height,
                       Orientation::HORIZONTAL);
  auto left = new LinearLayout(0, 0, 0, 0, Orientation::VERTICAL);
  left->addView(recyclerView, {0, 0, 1});

  auto *searchBox = new TextInputBox("assets/fonts/notosanscjkjp.ttf", 32);
  searchBox->setText("");
  searchBox->onSubmit([this, &context, searchBox](const std::string &text) {
    auto dbHelper = ChartDBHelper::GetInstance();
    if (text.empty()) {
      std::vector<bms_parser::ChartMeta> chartMetas;
      dbHelper.SelectAllChartMeta(db, chartMetas);
      recyclerView->setItems(chartMetas);
    } else {
      std::vector<bms_parser::ChartMeta> chartMetas;
      dbHelper.SearchChartMeta(db, text, chartMetas);
      recyclerView->setItems(chartMetas);
    }
  });
  left->addView(searchBox, {0, 50, 0});
  rootLayout->addView(left, {0, 0, 1});

  auto right = new LinearLayout(0, 0, 0, 0, Orientation::VERTICAL);
  right->setAlign(LinearLayout::CENTER);
  right->setPadding({12, 12, 12, 12});
  right->setGap(12);

  auto startButton = new Button(0, 0, 200, 100);
  auto buttonText = new TextView("assets/fonts/notosanscjkjp.ttf", 32);
  buttonText->setText("Start");
  buttonText->setAlign(TextView::CENTER);
  startButton->setContentView(buttonText);
  startButton->setOnClickListener([this, &context, buttonText]() {
    SDL_Log("Start button clicked");
    auto selected = recyclerView->selectedIndex;
    SDL_Log("Selected: %d", selected);
    if (selected >= 0) {
      willStart = true;
      buttonText->setText("Loading...");

      defer(
          [this, &context]() {
            SDL_Log("Starting game play scene");
            ImageView::dropAllCache();
            if (loadThread.joinable()) {
              loadThread.join();
            }
            context.jukebox.stop();
            context.sceneManager->changeScene(
                new GamePlayScene(context, selectedChart));
          },
          0, true);
    }
  });
  right->addView(jacketView, {150, 150, 0});
  right->addView(startButton, {0, 100, 0});
  rootLayout->addView(right, {200, 0, 0});
  addView(rootLayout);
}

void MainMenuScene::update(float dt) {
  // Update the scene logic
  // std::cout << "Updating Main Menu Scene, dt: " << dt << std::endl;
}

void MainMenuScene::renderScene() {
  // Render the scene
  // SDL_Log("Rendering Main Menu Scene");
  rootLayout->setSize(rendering::window_width, rendering::window_height);
  context.jukebox.render();
}

void MainMenuScene::cleanupScene() {
  // Cleanup resources when exiting the scene
  ChartDBHelper::GetInstance().Close(db);
  if (checkEntriesThread.joinable()) {
    checkEntriesThread.join();
  }

  if (loadThread.joinable()) {
    loadThread.join();
  }
}

void MainMenuScene::LoadCharts(ChartDBHelper &dbHelper, sqlite3 *db,
                               std::vector<path_t> &entries,
                               MainMenuScene &scene,
                               std::atomic_bool &isCancelled) {
  std::vector<bms_parser::ChartMeta> chartMetas;
  dbHelper.SelectAllChartMeta(db, chartMetas);
  // sort by title
  std::sort(chartMetas.begin(), chartMetas.end(),
            [](bms_parser::ChartMeta &a, bms_parser::ChartMeta &b) {
              return a.Title < b.Title;
            });
  std::vector<Diff> diffs;
  std::cout << "Finding new bms files" << std::endl;
  std::unordered_set<path_t> oldFilesWs;

  for (auto &chartMeta : chartMetas) {
    if (isCancelled) {
      break;
    }
    oldFilesWs.insert(fspath_to_path_t(chartMeta.BmsPath));
    // std::cout << "Old file: " << chartMeta.BmsPath << std::endl;
    // std::cout << "Folder: " << chartMeta.Folder << std::endl;
  }
  for (auto &entry : entries) {
    if (isCancelled) {
      break;
    }
    FindNewBmsFiles(diffs, oldFilesWs, entry, isCancelled);
  }

  std::cout << "Found " << diffs.size() << " new bms files" << std::endl;
  std::atomic_bool is_committing(false);
  std::atomic_int success_count(0);
  dbHelper.BeginTransaction(db);
  parallel_for(diffs.size(), [&](int start, int end) {
    for (int i = start; i < end; i++) {
      if (isCancelled) {
        break;
      }
      auto &diff = diffs[i];
      if (diff.type == Added) {
        bms_parser::Parser parser;
        bms_parser::Chart *chart;
        std::atomic_bool cancel(false);
        bms_parser::ChartMeta chartMeta;
        try {
          parser.Parse(diffs[i].path, &chart, false, true, cancel);
        } catch (std::exception &e) {
          delete chart;
          std::cerr << "Error parsing " << diffs[i].path << ": " << e.what()
                    << std::endl;
          continue;
        }

        if (chart == nullptr)
          continue;
        ++success_count;
        if (success_count % 1000 == 0 && !is_committing) {
          is_committing = true;
          dbHelper.CommitTransaction(db);
          dbHelper.BeginTransaction(db);
          is_committing = false;
        }
        dbHelper.InsertChartMeta(db, chart->Meta);
        delete chart;
      }
    }
  });
  dbHelper.CommitTransaction(db);
  std::cout << "Inserted " << success_count << " new charts" << std::endl;
  // set items
  chartMetas.clear();
  dbHelper.SelectAllChartMeta(db, chartMetas);

  scene.recyclerView->setItems(chartMetas);
}

#ifdef _WIN32
void MainMenuScene::FindFilesWin(const std::filesystem::path &path,
                                 std::vector<Diff> &diffs,
                                 const std::unordered_set<path_t> &oldFilesWs,
                                 std::vector<path_t> &directoriesToVisit,
                                 std::atomic_bool &isCancelled) {
  WIN32_FIND_DATAW findFileData;
  HANDLE hFind =
      FindFirstFileW((path.wstring() + L"\\*.*").c_str(), &findFileData);

  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      if (isCancelled) {
        FindClose(hFind);
        break;
      }
      if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        path_t filename(findFileData.cFileName);

        if (filename.size() > 4) {
          path_t ext = filename.substr(filename.size() - 4);
          std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
          if (ext == L".bms" || ext == L".bme" || ext == L".bml") {
            path_t dirPath;

            path_t fullPath = path.wstring() + L"\\" + filename;
            if (oldFilesWs.find(fullPath) == oldFilesWs.end()) {
              diffs.push_back({fullPath, Added});
            }
          }
        }
      } else if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        path_t filename(findFileData.cFileName);

        if (filename != L"." && filename != L"..") {
          directoriesToVisit.push_back(path.wstring() + L"\\" + filename);
        }
      }
    } while (FindNextFileW(hFind, &findFileData) != 0);
    FindClose(hFind);
  }
}
#elif TARGET_OS_OSX || TARGET_OS_LINUX
void MainMenuScene::resolveDType(const std::filesystem::path &directoryPath,
                                 struct dirent *entry) {
  if (entry->d_type == DT_UNKNOWN) {
    std::filesystem::path fullPath = directoryPath / entry->d_name;
    struct stat statbuf;
    if (stat(fullPath.c_str(), &statbuf) == 0) {
      if (S_ISREG(statbuf.st_mode)) {
        entry->d_type = DT_REG;
      } else if (S_ISDIR(statbuf.st_mode)) {
        entry->d_type = DT_DIR;
      }
    }
  }
}
// TODO: Use platform-specific method for faster traversal
void MainMenuScene::FindFilesUnix(
    const std::filesystem::path &directoryPath, std::vector<Diff> &diffs,
    const std::unordered_set<path_t> &oldFiles,
    std::vector<std::filesystem::path> &directoriesToVisit,
    std::atomic_bool &isCancelled) {
  DIR *dir = opendir(directoryPath.c_str());
  if (dir) {
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
      if (isCancelled) {
        closedir(dir);
        break;
      }
      resolveDType(directoryPath, entry);
      if (entry->d_type == DT_REG) {
        std::string filename = entry->d_name;
        if (filename.size() > 4) {
          std::string ext = filename.substr(filename.size() - 4);
          std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
          if (ext == ".bms" || ext == ".bme" || ext == ".bml") {
            std::filesystem::path fullPath = directoryPath / filename;
            if (oldFiles.find(fspath_to_path_t(fullPath)) == oldFiles.end()) {
              diffs.push_back({fullPath, Added});
            }
          }
        }
      } else if (entry->d_type == DT_DIR) {
        std::string filename = entry->d_name;
        if (filename != "." && filename != "..") {
          directoriesToVisit.push_back(directoryPath / filename);
        }
      } else {
        SDL_Log("Unknown file type: %s", entry->d_name);
      }
    }
    closedir(dir);
  } else {
    SDL_Log("Failed to open directory: %s", directoryPath.c_str());
  }
}

#elif TARGET_OS_IOS
void MainMenuScene::FindFilesIOS(
    const std::filesystem::path &path, std::vector<Diff> &diffs,
    const std::unordered_set<path_t> &oldFilesWs,
    std::vector<std::filesystem::path> &directoriesToVisit,
    std::atomic_bool &isCancelled) {
  // use iosnatives
  auto files = ListDocumentFilesRecursively();
  SDL_Log("Found %d files", files.size());
  for (auto &file : files) {
    // SDL_Log("File: %s", file.c_str());
    if (file.size() > 4) {
      std::string ext = file.substr(file.size() - 4);
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
      if (ext == ".bms" || ext == ".bme" || ext == ".bml") {
        path_t fullPath = GetIOSDocumentsPath() + "/" + file;
        if (oldFilesWs.find(fullPath) == oldFilesWs.end()) {
          diffs.push_back({fullPath, Added});
        }
      }
    }
  }
}
#endif

void MainMenuScene::FindNewBmsFiles(
    std::vector<Diff> &diffs, const std::unordered_set<path_t> &oldFilesWs,
    const std::filesystem::path &path, std::atomic_bool &isCancelled) {
#ifdef _WIN32
  std::vector<path_t> directoriesToVisit;
  directoriesToVisit.push_back(path.wstring());
#else
  std::vector<std::filesystem::path> directoriesToVisit;
  directoriesToVisit.push_back(path);
#endif
  SDL_Log("Finding new bms files in %s", path.c_str());
  while (!directoriesToVisit.empty()) {
    if (isCancelled) {
      break;
    }
    std::filesystem::path currentDir = directoriesToVisit.back();
    directoriesToVisit.pop_back();

#ifdef _WIN32

    FindFilesWin(currentDir, diffs, oldFilesWs, directoriesToVisit,
                 isCancelled);
#elif TARGET_OS_OSX || TARGET_OS_LINUX
    FindFilesUnix(currentDir, diffs, oldFilesWs, directoriesToVisit,
                  isCancelled);
#elif TARGET_OS_IOS
    FindFilesIOS(currentDir, diffs, oldFilesWs, directoriesToVisit,
                 isCancelled);
#endif
  }
}

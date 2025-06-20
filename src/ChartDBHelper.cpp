// Fill out your copyright notice in the Description page of Project Settings.

#include "ChartDBHelper.h"
#include "Utils.h"
#include <SDL2/SDL.h>
#include "path.h"

#include <codecvt>
#include <filesystem>
#include <iostream>
#include "targets.h"

sqlite3 *ChartDBHelper::Connect() {
  std::filesystem::path Directory = Utils::GetDocumentsPath("db");
  std::cout << "DB Directory: " << Directory.string() << "\n";
  std::filesystem::create_directories(Directory);
  std::filesystem::path path = Directory / "chart.db";
  std::cout << "DB Path: " << path.string() << "\n";
  sqlite3 *db;
  int rc;
  rc = sqlite3_open(path.string().c_str(), &db);
  sqlite3_busy_timeout(db, 1000);
  if (rc) {
    std::cerr << "Can't open database: " << sqlite3_errmsg(db) << "\n";
    sqlite3_close(db);
    return nullptr;
  }
  // wal
  sqlite3_exec(db, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr);
  sqlite3_exec(db, "PRAGMA synchronous=NORMAL", nullptr, nullptr, nullptr);
  return db;
}

void ChartDBHelper::Close(sqlite3 *db) { sqlite3_close(db); }

void ChartDBHelper::BeginTransaction(sqlite3 *db) {
  sqlite3_exec(db, "BEGIN", nullptr, nullptr, nullptr);
}

void ChartDBHelper::CommitTransaction(sqlite3 *db) {
  sqlite3_exec(db, "COMMIT", nullptr, nullptr, nullptr);
}

bool ChartDBHelper::CreateChartMetaTable(sqlite3 *db) {
  auto query = "CREATE TABLE IF NOT EXISTS chart_meta ("
               "path       TEXT primary key,"
               "md5        TEXT not null,"
               "sha256     TEXT not null,"
               "title      TEXT,"
               "subtitle   TEXT,"
               "genre      TEXT,"
               "artist     TEXT,"
               "sub_artist  TEXT,"
               "folder     TEXT,"
               "stage_file  TEXT,"
               "banner     TEXT,"
               "back_bmp    TEXT,"
               "preview    TEXT,"
               "level      REAL,"
               "difficulty INTEGER,"
               "total     REAL,"
               "bpm       REAL,"
               "max_bpm     REAL,"
               "min_bpm     REAL,"
               "length     INTEGER,"
               "rank      INTEGER,"
               "player    INTEGER,"
               "keys     INTEGER,"
               "total_notes INTEGER,"
               "total_long_notes INTEGER,"
               "total_scratch_notes INTEGER,"
               "total_backspin_notes INTEGER"
               ")";
  char *errMsg;
  int rc = sqlite3_exec(db, query, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error while creating chart meta table: " << errMsg
              << "\n";
    sqlite3_free(errMsg);
    return false;
  }
  return true;
}

bool ChartDBHelper::InsertChartMeta(sqlite3 *db,
                                    bms_parser::ChartMeta &chartMeta) {
  auto query = "REPLACE INTO chart_meta ("
               "path,"
               "md5,"
               "sha256,"
               "title,"
               "subtitle,"
               "genre,"
               "artist,"
               "sub_artist,"
               "folder,"
               "stage_file,"
               "banner,"
               "back_bmp,"
               "preview,"
               "level,"
               "difficulty,"
               "total,"
               "bpm,"
               "max_bpm,"
               "min_bpm,"
               "length,"
               "rank,"
               "player,"
               "keys,"
               "total_notes,"
               "total_long_notes,"
               "total_scratch_notes,"
               "total_backspin_notes"
               ") VALUES("
               "@path,"
               "@md5,"
               "@sha256,"
               "@title,"
               "@subtitle,"
               "@genre,"
               "@artist,"
               "@sub_artist,"
               "@folder,"
               "@stage_file,"
               "@banner,"
               "@back_bmp,"
               "@preview,"
               "@level,"
               "@difficulty,"
               "@total,"
               "@bpm,"
               "@max_bpm,"
               "@min_bpm,"
               "@length,"
               "@rank,"
               "@player,"
               "@keys,"
               "@total_notes,"
               "@total_long_notes,"
               "@total_scratch_notes,"
               "@total_backspin_notes"
               ")";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::string err = std::string(sqlite3_errmsg(db));
    // UE_LOG(LogTemp, Error, TEXT("SQL error while preparing statement to
    // insert a chart: %s"), *err);
    sqlite3_close(db);
    return false;
  }
  std::filesystem::path path = chartMeta.BmsPath;
  ToRelativePath(path);
  
  sqlite3_bind_text(stmt, 1, path_t_to_utf8(fspath_to_path_t(path)).c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, (chartMeta.MD5).c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 3, (chartMeta.SHA256).c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 4, (chartMeta.Title).c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 5, (chartMeta.SubTitle).c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 6, (chartMeta.Genre).c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 7, (chartMeta.Artist).c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 8, (chartMeta.SubArtist).c_str(), -1,
                    SQLITE_TRANSIENT);
  
  std::filesystem::path folder = chartMeta.Folder;
  ToRelativePath(folder);
  sqlite3_bind_text(stmt, 9, path_t_to_utf8(fspath_to_path_t(folder)).c_str(),
                    -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 10, path_t_to_utf8(fspath_to_path_t(chartMeta.StageFile)).c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 11, path_t_to_utf8(fspath_to_path_t(chartMeta.Banner)).c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 12, path_t_to_utf8(fspath_to_path_t(chartMeta.BackBmp)).c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 13, path_t_to_utf8(fspath_to_path_t(chartMeta.Preview)).c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_double(stmt, 14, chartMeta.PlayLevel);
  sqlite3_bind_int(stmt, 15, chartMeta.Difficulty);
  sqlite3_bind_double(stmt, 16, chartMeta.Total);
  sqlite3_bind_double(stmt, 17, chartMeta.Bpm);
  sqlite3_bind_double(stmt, 18, chartMeta.MaxBpm);
  sqlite3_bind_double(stmt, 19, chartMeta.MinBpm);
  sqlite3_bind_int64(stmt, 20, chartMeta.PlayLength);
  sqlite3_bind_int(stmt, 21, chartMeta.Rank);
  sqlite3_bind_int(stmt, 22, chartMeta.Player);
  sqlite3_bind_int(stmt, 23, chartMeta.KeyMode);
  sqlite3_bind_int(stmt, 24, chartMeta.TotalNotes);
  sqlite3_bind_int(stmt, 25, chartMeta.TotalLongNotes);
  sqlite3_bind_int(stmt, 26, chartMeta.TotalScratchNotes);
  sqlite3_bind_int(stmt, 27, chartMeta.TotalBackSpinNotes);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    SDL_Log("SQL error while inserting a chart: %s", sqlite3_errmsg(db));
    sqlite3_free(stmt);
    return false;
  }
  sqlite3_finalize(stmt);
  return true;
}

void ChartDBHelper::SelectAllChartMeta(
    sqlite3 *db, std::vector<bms_parser::ChartMeta> &chartMetas) {
  auto query = "SELECT "
               "path,"
               "md5,"
               "sha256,"
               "title,"
               "subtitle,"
               "genre,"
               "artist,"
               "sub_artist,"
               "folder,"
               "stage_file,"
               "banner,"
               "back_bmp,"
               "preview,"
               "level,"
               "difficulty,"
               "total,"
               "bpm,"
               "max_bpm,"
               "min_bpm,"
               "length,"
               "rank,"
               "player,"
               "keys,"
               "total_notes,"
               "total_long_notes,"
               "total_scratch_notes,"
               "total_backspin_notes"
               " FROM chart_meta ORDER BY title";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error while getting all charts: " << sqlite3_errmsg(db)
              << "\n";
    sqlite3_free(stmt);
    return;
  }

  // reserve space for the result
  chartMetas.reserve(sqlite3_column_count(stmt));

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    chartMetas.push_back(std::move(ReadChartMeta(stmt)));
  }
  sqlite3_finalize(stmt);
}

void ChartDBHelper::SearchChartMeta(
    sqlite3 *db, const std::string &text,
    std::vector<bms_parser::ChartMeta> &chartMetas) {
  auto query =
      "SELECT "
      "path,"
      "md5,"
      "sha256,"
      "title,"
      "subtitle,"
      "genre,"
      "artist,"
      "sub_artist,"
      "folder,"
      "stage_file,"
      "banner,"
      "back_bmp,"
      "preview,"
      "level,"
      "difficulty,"
      "total,"
      "bpm,"
      "max_bpm,"
      "min_bpm,"
      "length,"
      "rank,"
      "player,"
      "keys,"
      "total_notes,"
      "total_long_notes,"
      "total_scratch_notes,"
      "total_backspin_notes"
      " FROM chart_meta WHERE rtrim(title || ' ' || subtitle || ' ' || artist "
      "|| ' ' || sub_artist || ' ' || genre) LIKE @text GROUP BY sha256 ORDER "
      "BY title";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error while searching for charts: " << sqlite3_errmsg(db)
              << "\n";
    sqlite3_free(stmt);
    return;
  }
  // %text%
  sqlite3_bind_text(stmt, 1, ("%" + text + "%").c_str(), -1, SQLITE_TRANSIENT);

  // reserve space for the result
  chartMetas.reserve(sqlite3_column_count(stmt));

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    chartMetas.push_back(std::move(ReadChartMeta(stmt)));
  }
  sqlite3_finalize(stmt);
}

bool ChartDBHelper::DeleteChartMeta(sqlite3 *db, std::filesystem::path &path) {
  ToRelativePath(path);
  auto query = "DELETE FROM chart_meta WHERE path = @path";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error while preparing statement to delete a chart: "
              << sqlite3_errmsg(db) << "\n";
    sqlite3_free(stmt);
    return false;
  }
  sqlite3_bind_text(stmt, 1, path.string().c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "SQL error while deleting a chart: " << sqlite3_errmsg(db)
              << "\n";
    sqlite3_close(db);
    return false;
  }
  sqlite3_finalize(stmt);
  return true;
}

bool ChartDBHelper::ClearChartMeta(sqlite3 *db) {
  auto query = "DELETE FROM chart_meta";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error while clearing: " << sqlite3_errmsg(db) << "\n";
    sqlite3_free(stmt);
    return false;
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {

    std::cerr << "SQL error while clearing: " << sqlite3_errmsg(db) << "\n";
    sqlite3_free(stmt);
    return false;
  }
  sqlite3_finalize(stmt);
  return true;
}

bms_parser::ChartMeta ChartDBHelper::ReadChartMeta(sqlite3_stmt *stmt) {
  int idx = 0;
  bms_parser::ChartMeta chartMeta;
  auto t = ReadPath(stmt, idx++);

  std::filesystem::path path = std::filesystem::path(t);
  ToAbsolutePath(path);
  chartMeta.BmsPath = path;
  chartMeta.MD5 = std::string(
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, idx++)));
  chartMeta.SHA256 = std::string(
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, idx++)));
  chartMeta.Title = std::string(
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, idx++)));
  chartMeta.SubTitle = std::string(
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, idx++)));
  chartMeta.Genre = std::string(
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, idx++)));
  chartMeta.Artist = std::string(
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, idx++)));
  chartMeta.SubArtist = std::string(
      reinterpret_cast<const char *>(sqlite3_column_text(stmt, idx++)));
  auto folder_char = ReadPath(stmt, idx++);
  std::filesystem::path folder = std::filesystem::path(folder_char);
  ToAbsolutePath(folder);
  chartMeta.Folder = folder;
  auto stage_file_char = ReadPath(stmt, idx++);
  chartMeta.StageFile = std::filesystem::path(stage_file_char);
  auto banner_char = ReadPath(stmt, idx++);
  chartMeta.Banner = std::filesystem::path(banner_char);
  auto back_bmp_char = ReadPath(stmt, idx++);
  chartMeta.BackBmp = std::filesystem::path(back_bmp_char);
  auto preview_char = ReadPath(stmt, idx++);
  chartMeta.Preview = std::filesystem::path(preview_char);

  chartMeta.PlayLevel = sqlite3_column_double(stmt, idx++);
  chartMeta.Difficulty = sqlite3_column_int(stmt, idx++);
  chartMeta.Total = sqlite3_column_double(stmt, idx++);
  chartMeta.Bpm = sqlite3_column_double(stmt, idx++);
  chartMeta.MaxBpm = sqlite3_column_double(stmt, idx++);
  chartMeta.MinBpm = sqlite3_column_double(stmt, idx++);
  chartMeta.PlayLength = sqlite3_column_int64(stmt, idx++);
  chartMeta.Rank = sqlite3_column_int(stmt, idx++);
  chartMeta.Player = sqlite3_column_int(stmt, idx++);
  chartMeta.KeyMode = sqlite3_column_int(stmt, idx++);
  chartMeta.TotalNotes = sqlite3_column_int(stmt, idx++);
  chartMeta.TotalLongNotes = sqlite3_column_int(stmt, idx++);
  chartMeta.TotalScratchNotes = sqlite3_column_int(stmt, idx++);
  chartMeta.TotalBackSpinNotes = sqlite3_column_int(stmt, idx++);

  return chartMeta;
}

bool ChartDBHelper::CreateEntriesTable(sqlite3 *db) {
  // save paths to search for charts
  auto query = "CREATE TABLE IF NOT EXISTS entries ("
               "path       TEXT primary key"
               ")";

  char *errMsg;
  int rc = sqlite3_exec(db, query, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error while creating entries table: "
              << sqlite3_errmsg(db) << "\n";
    sqlite3_free(errMsg);
    return false;
  }
  return true;
}

bool ChartDBHelper::InsertEntry(sqlite3 *db, std::filesystem::path &path) {
  auto query = "REPLACE INTO entries ("
               "path"
               ") VALUES("
               "@path"
               ")";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error while preparing statement to insert an entry: "
              << sqlite3_errmsg(db) << "\n";
    sqlite3_close(db);
    return false;
  }
  sqlite3_bind_text(stmt, 1, path.string().c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "SQL error while inserting an entry: " << sqlite3_errmsg(db)
              << "\n";
    sqlite3_free(stmt);
    return false;
  }
  sqlite3_finalize(stmt);
  return true;
}

std::vector<path_t> ChartDBHelper::SelectAllEntries(sqlite3 *db) {
  auto query = "SELECT "
               "path"
               " FROM entries";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error while getting all entries: " << sqlite3_errmsg(db)
              << "\n";
    sqlite3_free(stmt);
    return std::vector<path_t>();
  }
  std::vector<path_t> entries;
  entries.reserve(sqlite3_column_count(stmt));
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    std::filesystem::path entry = std::filesystem::path(
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
    entries.push_back(fspath_to_path_t(entry));
  }
  sqlite3_finalize(stmt);
  return entries;
}

bool ChartDBHelper::DeleteEntry(sqlite3 *db, std::filesystem::path &path) {
  auto query = "DELETE FROM entries WHERE path = @path";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {
    std::cerr << "SQL error while preparing statement to delete an entry: "
              << sqlite3_errmsg(db) << "\n";
    sqlite3_free(stmt);
    return false;
  }
  sqlite3_bind_text(stmt, 1, path.string().c_str(), -1, SQLITE_TRANSIENT);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "SQL error while deleting an entry: " << sqlite3_errmsg(db)
              << "\n";
    sqlite3_close(db);
    return false;
  }
  sqlite3_finalize(stmt);
  return true;
}

bool ChartDBHelper::ClearEntries(sqlite3 *db) {
  auto query = "DELETE FROM entries";
  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
  if (rc != SQLITE_OK) {

    std::cerr << "SQL error while clearing: " << sqlite3_errmsg(db) << "\n";
    sqlite3_free(stmt);
    return false;
  }
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    std::cerr << "SQL error while clearing: " << sqlite3_errmsg(db) << "\n";
    sqlite3_free(stmt);
    return false;
  }
  sqlite3_finalize(stmt);
  return true;
}

void ChartDBHelper::ToRelativePath(
    [[maybe_unused]] std::filesystem::path &path) {
  // for iOS, remove Documents
#if TARGET_OS_IOS
  static std::filesystem::path Documents = Utils::GetDocumentsPath("BMS/");
  if (path.string().find(Documents.string()) != std::string::npos) {
    path = path.string().substr(Documents.string().length());
  }

#endif
  // otherwise, noop
}

void ChartDBHelper::ToAbsolutePath(
    [[maybe_unused]] std::filesystem::path &path) {
#if TARGET_OS_IOS
  static std::filesystem::path Documents = Utils::GetDocumentsPath("BMS/");
  path = Documents / path;
#endif
  // otherwise, noop
}

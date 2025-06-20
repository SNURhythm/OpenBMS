// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "bms_parser.hpp"
#include "path.h"
#include "sqlite3.h"
#include <filesystem>
#include <vector>
/**
 *
 */
class ChartDBHelper {
public:
  // Singleton
  ChartDBHelper() {}

  ChartDBHelper(const ChartDBHelper &) {}

  ChartDBHelper &operator=(const ChartDBHelper &) { return *this; }

  static ChartDBHelper &GetInstance() {
    sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    static ChartDBHelper instance;
    return instance;
  }

  // Connect, return connection
  sqlite3 *Connect();

  // CreateTable
  bool CreateChartMetaTable(sqlite3 *db);

  // Insert ChartMeta
  bool InsertChartMeta(sqlite3 *db, bms_parser::ChartMeta &chartMeta);
  void SelectAllChartMeta(sqlite3 *db,
                          std::vector<bms_parser::ChartMeta> &chartMetas);
  void SearchChartMeta(sqlite3 *db, const std::string &keyword,
                       std::vector<bms_parser::ChartMeta> &chartMetas);
  bool DeleteChartMeta(sqlite3 *db, std::filesystem::path &path);
  bool ClearChartMeta(sqlite3 *db);
  void Close(sqlite3 *db);
  void BeginTransaction(sqlite3 *db);
  void CommitTransaction(sqlite3 *db);
  bool CreateEntriesTable(sqlite3 *db);
  bool InsertEntry(sqlite3 *db, std::filesystem::path &path);
  std::vector<path_t> SelectAllEntries(sqlite3 *db);
  bool DeleteEntry(sqlite3 *db, std::filesystem::path &path);
  bool ClearEntries(sqlite3 *db);

  static void ToRelativePath(std::filesystem::path &path);
  static void ToAbsolutePath(std::filesystem::path &path);

private:
  bms_parser::ChartMeta ReadChartMeta(sqlite3_stmt *stmt);
  path_t ReadPath(sqlite3_stmt *stmt, int idx) {
#ifdef _WIN32
    const path_t t =
        reinterpret_cast<const wchar_t *>(sqlite3_column_text16(stmt, idx));
#else
    const path_t t =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, idx));
#endif
    return t;
  }
};

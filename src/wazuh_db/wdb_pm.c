/*
 * Wazuh SQLite integration
 * Copyright (C) 2016 Wazuh Inc.
 * June 06, 2016.
 *
 * This program is a free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "wdb.h"

static const char *SQL_INSERT_PM = "INSERT INTO pm_event (date_first, date_last, log, pci_dss, cis) VALUES (?, ?, ?, ?, ?);";
static const char *SQL_UPDATE_PM = "UPDATE pm_event SET date_last = ? WHERE log = ?;";

/* Get PCI_DSS requirement from log string */
static char* get_pci_dss(const char *string);

/* Get CIS requirement from log string */
char* get_cis(const char *string);

/* Insert policy monitoring entry. Returns ID on success or -1 on error. */
int wdb_insert_pm(int id_agent, const char *location, long int date, const char *log) {
    sqlite3 *db;
    sqlite3_stmt *stmt = NULL;
    int result;
    char *pci_dss;
    char *cis;
    char *name = wdb_agent_loc2name(location);

    if (!name)
        return -1;

    db = wdb_open_agent(id_agent, name);
    free(name);

    if (!db)
        return -1;

    if (wdb_prepare(db, SQL_INSERT_PM, -1, &stmt, NULL)) {
        debug1("%s: SQLite: %s", ARGV0, sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        return -1;
    }

    pci_dss = get_pci_dss(log);
    cis = get_cis(log);

    sqlite3_bind_int(stmt, 1, date);
    sqlite3_bind_int(stmt, 2, date);
    sqlite3_bind_text(stmt, 3, log, -1, NULL);
    sqlite3_bind_text(stmt, 4, pci_dss, -1, NULL);
    sqlite3_bind_text(stmt, 5, cis, -1, NULL);

    result = wdb_step(stmt) == SQLITE_DONE ? (int)sqlite3_last_insert_rowid(db) : -1;
    sqlite3_finalize(stmt);
    sqlite3_close_v2(db);
    free(pci_dss);
    free(cis);
    return result;
}

/* Update policy monitoring last date. Returns 0 on success or -1 on error. */
int wdb_update_pm(int id_agent, const char *location, const char *log, long int date_last) {
    sqlite3 *db;
    sqlite3_stmt *stmt = NULL;
    int result;
    char *name = wdb_agent_loc2name(location);

    if (!name)
        return -1;

    db = wdb_open_agent(id_agent, name);
    free(name);

    if (!db)
        return -1;

    if (wdb_prepare(db, SQL_UPDATE_PM, -1, &stmt, NULL)) {
        debug1("%s: SQLite: %s", ARGV0, sqlite3_errmsg(db));
        sqlite3_close_v2(db);
        return -1;
    }

    sqlite3_bind_int(stmt, 1, date_last);
    sqlite3_bind_int(stmt, 2, id_agent);
    sqlite3_bind_text(stmt, 3, log, -1, NULL);

    result = wdb_step(stmt) == SQLITE_DONE ? 0 : -1;
    sqlite3_finalize(stmt);
    sqlite3_close_v2(db);
    return result;
}

/* Get PCI_DSS requirement from log string */
char* get_pci_dss(const char *string) {
    size_t length;
    char *out = strstr(string, "{PCI_DSS: ");

    if (out) {
        out += 10;
        length = strcspn(out, "}");

        if (length < strlen(out)) {
            out = strdup(out);
            out[length] = '\0';
            return out;
        }
    }
        return NULL;
}

/* Get CIS requirement from log string */
char* get_cis(const char *string) {
    size_t length;
    char *out = strstr(string, "{CIS: ");

    if (out) {
        out += 6;
        length = strcspn(out, "}");

        if (length < strlen(out)) {
            out = strdup(out);
            out[length] = '\0';
            return out;
        }
    }
        return NULL;
}

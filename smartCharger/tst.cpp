#include <iostream>
#include <sqlite3.h>
#include "common.hpp"

#define DBNAME "sm.db"

struct sqlRetVal
{
  int day;
  int starthour;
  int startmin;
  int count;
};

int sql_selectCallback(void* data, int argc, char **argv, char **azColName)
{
   // *(int*)data = atoi(argv[0]);

  for(int i=0; i<argc; i++)
  {
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");

  struct sqlRetVal* srv = (sqlRetVal*) data;
  srv -> day = atoi(argv[0]);
  srv -> starthour = atoi(argv[1]);
  srv -> startmin = atoi(argv[2]);
  srv -> count = argc;
  return 0;
}

void sql_findNextDrivingInfo()
{
  sqlite3 *db;
  char *zErrMsg = 0;

  // Open database
  if( sqlite3_open(DBNAME, &db) )
  {
    ERR_MSG("Can't open database! SQLite3 said: " << sqlite3_errmsg(db));
    return;
  }

  char sql[] = "SELECT day, starthour, startmin FROM dc WHERE rowid = (SELECT MIN(rowid) FROM DC WHERE day>=0 AND starthour>=0 AND startmin>10 AND distance>=10)";
  struct sqlRetVal srv;
  if( sqlite3_exec(db, sql, sql_selectCallback, (void*)&srv, &zErrMsg) != SQLITE_OK )
  {
    ERR_MSG("SQL SELECT error! SQLite3 said: " << zErrMsg);
    sqlite3_free(zErrMsg);
  }
  sqlite3_close(db);

  std::cout << srv.day << "\n" << srv.starthour << "\n" <<
               srv.startmin << "\n" << srv.count << std::endl;

  return;
}

int main(int argc, char const *argv[])
{
  sql_findNextDrivingInfo();
}






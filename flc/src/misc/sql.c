/*
  simple wrapper for ODBC or libmysql
  
  Copyright (c) 2006 Dirk
                            
                            
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifdef  HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include "sql.h"
#ifdef  USE_MYSQL
#include "sql_mysql.h"
#endif
#ifdef  USE_ODBC
#include "sql_odbc.h"
#endif


char *
sql_escape_string (char *s)
{
#if 1
 char *bak = strdup (s);
 char *p = bak;
 char *d = s;

 if (!p)
   return NULL;

 for (; *p; p++)
   switch (*p)
     {
       case 10:  // \n
         strcpy (d, "\\n");
         d = strchr (d, 0);
         break;

       case 13:  // \r
         strcpy (d, "\\r");
         d = strchr (d, 0);
         break;

       case 34:  // quotes
       case 39:  // single quotes
       case 92:  // backslash
         sprintf (d, "\\\%c", *p);
         d = strchr (d, 0);
         break;

       default:
//         if (*p > 31 && *p < 127)
           {
             *d = *p;
             *(++d) = 0;
           }
         break;
     }

  free (bak);
#else
  strrep (s, "\n", "\\n");
  strrep (s, "\r", "\\r");
  strrep (s, "\"", "\\\"");
  strrep (s, "\'", "\\\'");
  strrep (s, "\\", "\\");
#endif

  return s;
}


#if     (defined USE_ODBC || defined USE_MYSQL)
st_sql_t *
sql_open (const char *host, int port,
          const char *db, const char *user,
          const char *password, int flags)
{
  static st_sql_t sql;

  if (!(flags & SQL_MYSQL) &&
      !(flags & SQL_ODBC))
    return NULL;

  memset (&sql, 0, sizeof (st_sql_t));

  sql.flags = flags;

#ifdef  USE_MYSQL
  if (flags & SQL_MYSQL)
    return sql_mysql_open (&sql, host, port, db, user, password);
#endif
#ifdef  USE_ODBC
  if (flags & SQL_ODBC)
    return sql_odbc_open (&sql, host, port, db, user, password);
#endif

  return NULL;
}

int
sql_query (st_sql_t *sql, const char *query)
{
#ifdef  USE_MYSQL
  if (sql->flags & SQL_MYSQL)
    return sql_mysql_query (sql, query);
#endif
#ifdef  USE_ODBC
  if (sql->flags & SQL_ODBC)
    return sql_odbc_query (sql, query);
#endif

  return 0;
}


char *
sql_gets (st_sql_t *sql, char *buf, int buf_len)
{
#ifdef  USE_MYSQL
  if (sql->flags & SQL_MYSQL)
    return sql_mysql_gets (sql, buf, buf_len);
#endif
#ifdef  USE_ODBC
  if (sql->flags & SQL_ODBC)
    return sql_odbc_gets (sql, buf, buf_len);
#endif

  return 0;
}


int
sql_close (st_sql_t *sql)
{
#ifdef  USE_MYSQL
  if (sql->flags & SQL_MYSQL)
    return sql_mysql_close (sql);
#endif
#ifdef  USE_ODBC
  if (sql->flags & SQL_ODBC)
    return sql_odbc_close (sql);
#endif

  return 0;
}


//#if 1
#ifdef  TEST
int
main (int argc, char *argv[])
{
  st_sql_t *sql = NULL;

  if (!(sql = sql_open ("localhost", 3306, "mysql", "root", "", SQL_MYSQL)))
    return -1;

  sql_query (sql, "SELECT * FROM user");
  sql_query (sql, "SELECT * FROM user");
  
  sql_close (sql);

  return 0;
}
#endif  // TEST
#endif  // #if     (defined USE_ODBC || defined USE_MYSQL)

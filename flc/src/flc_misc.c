/*
flc_misc.c - miscellaneous functions for flc

Copyright (C) 1999-2002 by NoisyB

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifdef  HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef  HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef  HAVE_DIRENT_H
#include <dirent.h>
#endif
#include <ctype.h>
#include "misc/itypes.h"
#include "misc/file.h"
#include "misc/filter.h"
#include "misc/hash.h"
#include "misc/property.h"
#include "misc/string.h"
#include "misc/sql.h"
#include "flc.h"
#include "flc_misc.h"
#include "flc_defines.h"


#include "filter/id3.h"
#include "filter/met.h"
#include "filter/txt.h"


#ifndef TRUE
#define FALSE 0
#define TRUE !FALSE
#endif


#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif


const st_filter_t *filter[] = {
  &id3_filter,
  &met_filter,
  &txt_filter,
  NULL
};


int
system2 (const char *cmdline)
{
  int result = system (cmdline)
#ifndef __MSDOS__
      >> 8                                      // the exit code is coded in bits 8-15
#endif                                          //  (that is, under non-DOS)
      ;

  return result;
}


const st_filter_t *
get_filter_by_fname (const char *fname)
{
  const char *p = get_suffix (fname);
  if (p)
    {
      char suffix[MAXBUFSIZE];
      st_filter_chain_t *filter_chain = filter_malloc_chain (filter);
      const st_filter_t *filter = NULL;

      strcpy (suffix, p);
      strlwr (suffix);

      filter = filter_get_filter_by_magic (filter_chain, (const unsigned char *) suffix, strlen (suffix));
      filter_free_chain (filter_chain);
      return filter;
    }
  return NULL;
}


int
output (FILE *fp, const st_file_t *file)
{
  int x;
  char buf[MAXBUFSIZE];

  if (!file)
    return -1;
    
  if (flc.flags & FLC_HTML)
    {
      int len = strlen (basename2 (file->fname));

      strcpy (buf, basename2 (file->fname));
      buf[12] = 0;
      fprintf (fp, "<a href=\"%s\">%s</a>", file->fname, buf);

      if (len < 12)
        {
          strcpy (buf, "              ");
          buf[12 - len] = 0;
          fprintf (fp, buf);
        }
    }
  else
    fprintf (fp, "%-12.12s", basename2 (file->fname));

  fprintf (fp, " %c", file->checked ? file->checked : 'N');

  if (flc.flags & FLC_KBYTE && file->size > 1024)
    sprintf (buf, "%6luk", file->size / 1024);
  else
    sprintf (buf, "%7lu", file->size);
  strcat (buf, "                                            ");
  fprintf (fp, "%-9.9s", buf);

  strftime (buf, 11, "%m-%d-%Y   ", localtime ((const time_t *)&file->date));
  buf[6] = buf[8];
  buf[7] = buf[9];
  buf[8] = 0;
  fprintf (fp, "%-10.10s", buf);

  if (file->file_id[0][0])
    for (x = 0; file->file_id[x][0]; x++)
      fprintf (fp, !x ? "%-45.45s\n" : "                                 %-45.45s\n", file->file_id[x]);
  else
    fprintf (fp, "%-45.45s\n", basename2 (file->fname));

  return 0;
}


int
output_sql (FILE *fp, const st_file_t *file)
{
  st_hash_t *h = NULL;
  int i = 0, j = 0;
  st_file_t f;
  char buf[FLC_MAX_ID_ROWS * (FLC_MAX_ID_COLS + 1)];

  fputs ("---------------------------------------------------------------------------------\n"
         "-- flc - create BBS-style file lists with FILE_ID.DIZ found in archives and files\n"
         "---------------------------------------------------------------------------------\n"
         "\n"
         "-- DROP TABLE IF EXISTS `flc_table`;\n"
         "-- CREATE TABLE IF NOT EXISTS `flc_table`\n"
         "-- (\n"
         "--   `flc_md5`        varchar(32),\n"
         "--   `flc_crc32`      varchar(32),\n"
         "--   `flc_fname`      text,\n"
         "--   `flc_size`       int(32),\n"
         "--   `flc_checked`    varchar(3),\n"
         "--   `flc_date`       int(11),\n"
         "--   `flc_file_id`    longtext,\n"
         "--   `flc_date_added` int(11),\n"
         "--   UNIQUE KEY       `flc_md5` (`flc_md5`),\n"
         "--   UNIQUE KEY       `flc_crc32` (`flc_crc32`)\n"
         "-- );\n"
         "\n", fp);

  for (i = 0; i < flc.files; i++)
    {
      memcpy (&f, &file[i], sizeof (st_file_t));

      if (f.file_id[0][0])
        { 
          *buf = 0;
          for (j = 0; f.file_id[j][0]; j++)
            sprintf (strchr (buf, 0), "%-45.45s\n", f.file_id[j]);
        }
      else
        sprintf (buf, "%-45.45s\n", basename2 (file->fname));

      h = hash_open (HASH_MD5|HASH_CRC32);
      h = hash_update (h, (const unsigned char *) f.fname, strlen (file[i].fname));

      fprintf (fp, "INSERT INTO `flc_table` (`flc_md5`, `flc_crc32`, `flc_fname`, `flc_size`, `flc_checked`, `flc_date`, `flc_file_id`, `flc_date_added`)"
               " VALUES ('%s', '%x', '%s', '%ld', '%c', '%ld', '%s', '%ld')"
               ";\n",
        hash_get_s (h, HASH_MD5),
        hash_get_crc32 (h),
        sql_stresc (f.fname),
        f.size,
        f.checked,
        f.date,
        sql_stresc (buf),
        time (0));

      hash_close (h);
    }

  return 0;
}


static int
input_from_file (const char *fname, st_file_t *file)
// parse file_id.diz into st_file_t
{
#if     FILENAME_MAX > MAXBUFSIZE
  char buf[FILENAME_MAX];
#else
  char buf[MAXBUFSIZE];
#endif
  int x = 0;
  char *s = NULL;
  struct stat puffer;
  FILE *fp = NULL;
  
  file->file_id[0][0] = 0;

  if (!(fp = fopen (fname, "rb")))
    return -1;

  for (; x < FLC_MAX_ID_ROWS; x++)
    {
      if (!(fgets (buf, MAXBUFSIZE, fp)))
        break;

      if ((s = strpbrk (buf, "\x0a\x0d"))) // strip any returns
        *s = 0;

      buf[46] = 0;
      strcpy (file->file_id[x], *buf ? buf : " ");
    }
    
  fclose (fp);
  file->file_id[x][0] = 0;
              
  if (!stat (fname, &puffer))
    if (S_ISREG (puffer.st_mode))
      if (puffer.st_mtime < file->date) // update file->date with date from file_id.diz
        file->date = puffer.st_mtime;

  return 0;
}


static int
input_from_filelist (const char *fname, st_file_t *file)
// parse file_id.diz from filelist into st_file_t
{
#if     FILENAME_MAX > MAXBUFSIZE
  char buf[FILENAME_MAX];
#else
  char buf[MAXBUFSIZE];
#endif
  int x = 0;
  int found = 0;
  FILE *fp = NULL;
  const char *p = basename2 (fname);
  char *s = NULL;
  
  file->file_id[0][0] = 0;

  if (!(fp = fopen (flc.cache, "rb")))
    return -1;

  while ((fgets (buf, MAXBUFSIZE, fp)))
    {
      if (!strnicmp (buf, p, MIN (strlen (p), 12)))
        {
          found = 1;

          if ((s = strpbrk (buf, "\x0a\x0d"))) // strip any returns
            *s = 0;

          if ((s = (char *) memmem2 (buf, strlen (buf), "xx-xx-xx  ", 10, MEMMEM2_WCARD ('x'))))
            {
              struct tm t;

              memset (&t, 0, sizeof (struct tm));

              t.tm_year = strtol (s + 6, NULL, 10);
              *(s + 5) = 0;
              t.tm_mday = strtol (s + 3, NULL, 10);
              *(s + 2) = 0;
              t.tm_mon = strtol (s, NULL, 10) - 1;

              file->date = mktime (&t);
            }
          
          strcpy (file->file_id[0], &buf[33]);
          for (x = 1; x < FLC_MAX_ID_ROWS; x++)
            {
              if (!(fgets (buf, MAXBUFSIZE, fp)))
                break;

              if ((s = strpbrk (buf, "\x0a\x0d"))) // strip any returns
                *s = 0;

              if (!isspace (*buf))
                break;
                
              buf[34 + 46] = 0;
              strcpy (file->file_id[x], *buf ? &buf[33] : " ");
            }
          break;
        }
    }
              
  fclose (fp);

  file->file_id[x][0] = 0;

  return found ? 0 : -1;
}


int
extract (st_file_t *file, const char *fname)
{
  const char *prop = NULL;
#if     FILENAME_MAX > MAXBUFSIZE
  char buf[FILENAME_MAX];
  char tmpfile[FILENAME_MAX];
  char suffix[FILENAME_MAX];
#else
  char buf[MAXBUFSIZE];
  char tmpfile[MAXBUFSIZE];
  char suffix[MAXBUFSIZE];
#endif
  int found = 0;
  const st_filter_t *f[2];
  struct stat puffer;
  DIR *dp = NULL;
  struct dirent *ep = NULL;
  const char *p = NULL;
  memset (file, 0, sizeof (st_file_t));

  if (stat (fname, &puffer) == -1)
    return -1; 
  if (S_ISREG (puffer.st_mode) != TRUE)
    return -1;

  *suffix = 0;
  p = get_suffix (fname);
  if (p)
    {
      strcpy (suffix, p + 1); // skip '.'
      strlwr (suffix);
    }

  // defaults
  f[0] = get_filter_by_fname (fname);
  f[1] = NULL;
  file->file_id[0][0] = 0;
  file->checked = 'N';
  file->date = puffer.st_mtime;
  file->size = puffer.st_size;
  strncpy (file->fname, fname, FILENAME_MAX)[FILENAME_MAX - 1] = 0;
  if (flc.flags & FLC_CHECK)
    {
      // test with configfile
      sprintf (buf, "%s_test", suffix);
      if ((prop = get_property (flc.configfile, buf, PROPERTY_MODE_TEXT)))
        if (*prop)
          {
            sprintf (buf, prop, fname);
            file->checked = !system2 (buf) ? 'P' : 'F'; // failed
          }

#if 0
      // try build-in formats
      if (file->checked == 'N')
        if (f[0])
          if (f[0]->write)
            file->checked = !f[0]->write (flc) ? 'P' : 'F'; // failed
#endif
    }

  // extract with configfile
  sprintf (buf, "%s_extract", suffix);
  if ((prop = get_property (flc.configfile, buf, PROPERTY_MODE_TEXT)))
    if (*prop)
      {
        chdir (flc.temp);

        sprintf (buf, prop, fname);
        system2 (buf);

        // find the file_id.diz
        if ((dp = opendir (flc.temp)))
          {
            while ((ep = readdir (dp)) != 0)
              if (!stat (ep->d_name, &puffer))
                if (S_ISREG (puffer.st_mode))
                  {
                    sprintf (tmpfile, "%s/%s", flc.temp, ep->d_name);
          
                    found = 1;
                    break;
                  }
             closedir (dp);
           }
        chdir (flc.cwd);
      }

  // try build-in formats
  if (!found)
    if (f[0])
      if (f[0]->open)
        {
          st_filter_chain_t *fc = NULL;

          sprintf (tmpfile, "%s/file_id.diz", flc.temp);
          flc.srcfile = file->fname;
          flc.dstfile = tmpfile;
          if ((fc = filter_malloc_chain (f)))
            {
              found = 1;
              filter_open (fc, (void *) &flc);
              filter_close (fc, (void *) &flc);
              filter_free_chain (fc);
            }
        }

  if (found)
    if (input_from_file (tmpfile, file) != 0)
      found = 0;

  // try cache
  if (!found)
    if (*flc.cache) 
      {
        if (!input_from_filelist (fname, file))
          found = 1;
      }

  // the filename
  if (!found)
    {
      strncpy (file->file_id[0], basename2 (file->fname), 46)[45] = 0; // default
      file->file_id[1][0] = 0;
    }

  if ((dp = opendir (flc.temp)))
    {
      while ((ep = readdir (dp)) != 0)
        {
          sprintf (tmpfile, "%s/%s", flc.temp, ep->d_name);
          if (!stat (tmpfile, &puffer))
            if (S_ISREG (puffer.st_mode))
              remove (tmpfile);
        }
      closedir (dp);
    }
        
  return 0;
}


static int
compare (const void *a, const void *b)
{
  int result = 0;
  st_file_t *p = (st_file_t *) a, *p2 = (st_file_t *) b;

  if (flc.flags & FLC_DATE)
    {
      if (p->date > p2->date)
        result = 1;
    }
  else if (flc.flags & FLC_SIZE)
    {
      if (p->size > p2->size)
        result = 1;
    }
  else if (flc.flags & FLC_NAME)
    {
      if ((basename2 (p->fname))[0] > (basename2 (p2->fname))[0])
        result = 1;
    }
  
  if (flc.flags & FLC_FR)
    result = !result;
  
  return result;
}


int
sort (st_file_t * file)
{
  qsort (file, flc.files, sizeof (st_file_t), compare);

  return 0;
}

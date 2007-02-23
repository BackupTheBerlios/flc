/*
flc
shows the FILE_ID.DIZ of archives/files
useful for FTP admins or people who have a lot to do with FILE_ID.DIZ

Copyright (C) 1999-2004 by NoisyB

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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#ifdef  HAVE_DIRENT_H
#include <dirent.h>
#endif
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "misc/itypes.h"
#include "misc/getopt.h"
#include "misc/getopt2.h"
#include "misc/property.h"
#include "misc/file.h"
#include "misc/filter.h"
#include "flc.h"
#include "flc_defines.h"
#include "flc_misc.h"

static void flc_exit (void);

st_flc_t flc; // workflow
static st_file_t file[FLC_MAX_FILES];


void
flc_exit (void)
{
  chdir (flc.cwd);

#if 0
  if (flc.temp[0])
#if 0
    rmdir2 (flc.temp);
#else
    rmdir (flc.temp);
#endif
#endif
}


static int
flc_file_handler (const char *path)
{
  if (flc.files > FLC_MAX_FILES)
    {
      fprintf (stderr, "ERROR: can not open more than %d inputs\n\n", FLC_MAX_FILES);
      return -1; // skip
    }
    
  if (!extract (&file[flc.files], path))
    flc.files++;

  return 0;
}


int
main (int argc, char *argv[])
{
#if     FILENAME_MAX > MAXBUFSIZE
//  char path[FILENAME_MAX];
#else
//  char path[MAXBUFSIZE];
#endif
  char short_options[ARGS_MAX];
  struct option long_only_options[ARGS_MAX];
//  uint32_t flags = 0;
  int x = 0, c;
  FILE *fh = NULL;
  int option_index = 0;
  int result = 0;
//  const st_getopt2_t *p = NULL;
  st_property_t props[] = {
    {
      "lha_test", "lha t \"%s\"",
      "LHA support\n"
      "%s will be replaced with the file/archive name"
    },
    {
      "lha_extract", "lha efi \"%s\" " FLC_ID_NAMES,
      NULL
    },
    {
      "lzh_test", "lha t \"%s\"",
      "LZH support"
    },
    {
      "lzh_extract", "lha efi \"%s\" " FLC_ID_NAMES,
      NULL
    },
    {
      "zip_test", "unzip -t \"%s\"",
      "ZIP support"
    },
    {
      "zip_extract", "unzip -xojC \"%s\" " FLC_ID_NAMES,
      NULL
    },
    {
      "rar_test", "unrar t \"%s\"",
      "RAR support"
    },
    {
      "rar_extract", "unrar x \"%s\" " FLC_ID_NAMES,
      NULL
    },
    {
      "ace_test", "unace t \"%s\"",
      "ACE support"
    },
    {
      "ace_extract", "unace e \"%s\" " FLC_ID_NAMES,
      NULL
    },
    {NULL, NULL, NULL}
  };
  const st_getopt2_t options[] = {
    {
      NULL,      0, 0, 0, NULL,
      "flc " FLC_VERSION_S " " CURRENT_OS_S " 1999-2006 by NoisyB\n"
      "This may be freely redistributed under the terms of the GNU Public License\n\n"
      "create BBS-style file lists with FILE_ID.DIZ found in archives and files\n\n"
      "Usage: flc [OPTION]... [FILE]...\n"
    },
    {
      "t",       0, 0, 't',
      NULL,      "sort by modification time"
    },
    {
      "X",       0, 0, 'X',
      NULL,      "sort alphabetical"
    },
    {
      "S",       0, 0, 'S',
      NULL,      "sort by byte size"
    },
    {
      "fr",      0, 0, 2,
      NULL,   "sort reverse"
    },
    {
      "k",       0, 0, 'k',
      NULL,   "show sizes in kilobytes"
    },
    {
      "bbs",     0, 0, 4,
      NULL,   "output as BBS style filelisting (default)"
    },
    {
      "html",    0, 0, 3,
      NULL,   "output as HTML document with links to the files"
    },
    {
      "sql",     0, 0, 5,
      NULL,   "output as ANSI SQL script"
    },
    {
      "o",       1, 0, 'o',
      "FILE", "write output into FILE"
    },
    {
      "c",       0, 0, 'c',
      NULL,   "also check every archive for errors\n"
              "return flags: N=not checked (default), P=passed, F=failed"
    },
    {
      "cache",   1, 0, 'C',
      "CACHE", "get a default file_id.diz from CACHE filelisting\n"
               "(searches for filename)"
    },
#if 0
    {
      "stats",   1, 0, 's',
      "LIST", "show stats of fileLISTing"
    },
#endif
    {
      "R",       0, 0, 'R',
      NULL,   "scan subdirectories recursively"
    },
    {
      "version", 0, 0, 'v',
      NULL,   "output version information and exit"
    },
    {
      "ver",     0, 0, 'v',
      NULL,   NULL
    },
    {
      "help",    0, 0, 'h',
      NULL,   "display this help and exit"
    },
    {
      "h",       0, 0, 'h',
      NULL,   NULL
    },
    {
      NULL,      0, 0, 0,
      NULL,      "\nAmiga version: noC-flc Version v1.O (File-Listing Creator) - (C)1994 nocTurne deSign/MST\n"
                 "Report problems to ucon64-main@lists.sf.net or go to http://ucon64.sf.net\n"
    },
    {NULL, 0, 0, 0, NULL, NULL}
  };

  memset (&flc, 0, sizeof (st_flc_t));

  realpath2 (PROPERTY_HOME_RC ("flc"), flc.configfile);

  result = property_check (flc.configfile, FLC_CONFIG_VERSION, 1);
  if (result == 1) // update needed
    result = set_property_array (flc.configfile, props);
  if (result == -1) // property_check() or update failed
    return -1;

  getcwd (flc.cwd, FILENAME_MAX);
  atexit (flc_exit);
                      
  getopt2_short (short_options, options, ARGS_MAX);
  getopt2_long_only (long_only_options, options, ARGS_MAX);

  while ((c = getopt_long_only (argc, argv, short_options, long_only_options, &option_index)) != -1)
    switch (c)
      {
        case 5:
          flc.flags |= FLC_SQL;
          break;

        case 4:
          flc.flags |= FLC_BBS;
          break;

        case 't':
          flc.flags |= (FLC_SORT|FLC_DATE);
          break;

        case 'X':
          flc.flags |= (FLC_SORT|FLC_NAME);
          break;

        case 'S':
          flc.flags |= (FLC_SORT|FLC_SIZE);
          break;

        case 2:
          flc.flags |= (FLC_SORT|FLC_FR);
          break;

        case 'k':
          flc.flags |= FLC_KBYTE;
          break;

        case 3:
          flc.flags |= FLC_HTML;
          break;

        case 'c':
          flc.flags |= FLC_CHECK;
          break;

        case 'R':
          flc.flags |= FLC_RECURSIVE;
          break;

        case 'C':
          if (optarg)
            strncpy (flc.cache, optarg, FILENAME_MAX)[FILENAME_MAX - 1] = 0;
          break;

        case 'o':
          if (optarg)
            strncpy (flc.output, optarg, FILENAME_MAX)[FILENAME_MAX - 1] = 0;
          break;
          
        case 'h':
          getopt2_usage (options);
          return 0;

        case 'v':
          printf ("%s\n", FLC_VERSION_S);
          return 0;

        default:
          printf ("Try '%s --help' for more information.\n", argv[0]);
          return -1;
      }

  if (argc < 2 || !optind)
    {
      getopt2_usage (options);
      exit (-1);
    }

  strcpy (flc.temp, "flc");
  if (!tmpnam3 (flc.temp, 1))
    {
      fputs ("ERROR: could not create temp dir\n", stderr);
      exit (-1);
    }

  flc.files = 0;

  if (!getfile (argc, argv, flc_file_handler, (GETFILE_FILES_ONLY | // extract works only for files
                   (flc.flags & FLC_RECURSIVE ? GETFILE_RECURSIVE : 0)))) // recursively?
    {
      getopt2_usage (options);
      exit (-1);
    }

  if (flc.flags & FLC_SORT)
    sort (file);

  if (flc.output[0])
    fh = fopen (flc.output, "wb");
  if (!fh)
    fh = stdout;

  if (flc.flags & FLC_SQL)
    {
      output_sql (fh, file);
    }
  else
    {
      if (flc.flags & FLC_HTML)
        fprintf (fh, "<html><head><title></title></head><body><pre><tt>");

      for (x = 0; x < flc.files; output (fh, &file[x++]));

      if (flc.flags & FLC_HTML)
        fprintf (fh, "</pre></tt></body></html>\n");
    }

  if (fh != stdout)
    fclose (fh);

  return 0;
}

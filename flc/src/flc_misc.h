/*
flc_misc.h - miscellaneous functions for flc

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
#ifndef FLC_MISC_H
#define FLC_MISC_H
extern int extract (st_file_t * file, const char *fname);
extern int sort (st_file_t * file);
extern int output (FILE *fp, const st_file_t *file);
extern int output_sql (FILE *fp, const st_file_t *file);
extern const st_filter_t *filter[];
#endif // FLC_MISC_H

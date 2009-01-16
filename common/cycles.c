
// MD5DEEP
//
// By Jesse Kornblum
//
// This is a work of the US Government. In accordance with 17 USC 105,
// copyright protection is not available for any work of the US Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// $Id$

#include "main.h"


typedef struct dir_table {
  TCHAR *name;
  struct dir_table *next;
} dir_table;

dir_table *my_table = NULL;


// Debugging code
/*
static void dump_table(void)
{
  struct dir_table *t = my_table;
  printf ("\nTable contents:\n");
  while (t != NULL)
  {
    display_filename(stdout,t->name);
    printf ("\n");
    t = t->next;
  }
  print_status ("-- end of table --\n");
}
*/



int done_processing_dir(TCHAR *fn)
{
  if (NULL == fn)
    return TRUE;

  dir_table *last, *temp;
  TCHAR *d_name = (TCHAR *)malloc(sizeof(TCHAR) * PATH_MAX);

#ifdef _WIN32
  _wfullpath(d_name,fn,PATH_MAX);
#else
  realpath(fn,d_name);
#endif

  if (my_table == NULL)
  {
    internal_error("Table is NULL in done_processing_dir");

    // This code never gets executed... 
    free(d_name);
    return FALSE;
  }

  temp = my_table;

  if (!_tcsncmp(d_name,temp->name,PATH_MAX))
  {
    my_table = my_table->next;
    free(temp->name);
    free(temp);
    free(d_name);
    return TRUE;
  }

  while (temp->next != NULL)
  {
    last = temp;
    temp = temp->next;
    if (!_tcsncmp(d_name,temp->name,PATH_MAX))
    {
      last->next = temp->next;
      free(temp->name);
      free(temp);
      free(d_name);
      return TRUE;
    }
  }

  internal_error("%s: Directory %s not found in done_processing_dir",
		 __progname, d_name);

  // This code never gets executed... 
  //  free (d_name);
  return FALSE;
}




int processing_dir(TCHAR *fn)
{
  if (NULL == fn)
    return TRUE;

  dir_table *new, *temp;
  TCHAR *d_name = (TCHAR *)malloc(sizeof(TCHAR) * PATH_MAX);

#ifdef _WIN32
  _wfullpath(d_name,fn,PATH_MAX);
#else
  realpath(fn,d_name);
#endif

  if (my_table == NULL)
  {
    my_table = (dir_table*)malloc(sizeof(dir_table));
    my_table->name = _tcsdup(d_name);
    my_table->next = NULL;

    free(d_name);
    return TRUE;
  }
 
  temp = my_table;

  while (temp->next != NULL)
  {
    // We should never be adding a directory that is already here 
    if (!_tcsncmp(temp->name,d_name,PATH_MAX))
    {
      internal_error("%s: Attempt to add existing %s in processing_dir",
		     __progname, d_name);
      // Does not execute
      free(d_name);
      return FALSE;
    }
    temp = temp->next;       
  }

  new = (dir_table*)malloc(sizeof(dir_table));
  new->name = _tcsdup(d_name);
  new->next = NULL;  
  temp->next = new;

  free(d_name);
  return TRUE;
}


int have_processed_dir(TCHAR *fn)
{
  if (NULL == fn)
    return TRUE;

  dir_table *temp;
  TCHAR *d_name;

  if (my_table == NULL)
    return FALSE;

  d_name = (TCHAR *)malloc(sizeof(TCHAR) * PATH_MAX);
#ifdef _WIN32
  _wfullpath(d_name,fn,PATH_MAX);
#else
  realpath(fn,d_name);
#endif

  temp = my_table;
  while (temp != NULL)
  {
    if (!_tcsncmp(temp->name,d_name,PATH_MAX))
    {
      free(d_name);
      return TRUE;
    }

    temp = temp->next;       
  }

  free(d_name);
  return FALSE;
}





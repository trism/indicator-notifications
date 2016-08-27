/*
 * Functions for tokenizing a string and marking the urls.
 */

#ifndef __URLREGEX_H__
#define __URLREGEX_H__

#include <glib.h>

typedef enum {
  MATCHED,
  NOT_MATCHED
} MatchType;

typedef struct {
  char       *text;
  char       *expanded;
  MatchType   type;
} MatchGroup;

void   urlregex_init(void);
guint  urlregex_count(void);
GList *urlregex_split(const char *text, guint index);
GList *urlregex_split_all(const char *text);

MatchGroup *urlregex_matchgroup_new(const char *text, const char *expanded, MatchType type);
void        urlregex_matchgroup_free(MatchGroup *group);
void        urlregex_matchgroup_list_free(GList *list);

#endif

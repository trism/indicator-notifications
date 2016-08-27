#include <string.h>
#include "urlregex.h"

#define LP_BUG_BASE_URL "https://bugs.launchpad.net/bugs/"
#define HTTP_BASE_URL "http://"
#define MAILTO_BASE_URL "mailto:"

/* Adapted from src/terminal-screen.c in the gnome-terminal source */

#define USERCHARS "-[:alnum:]"
#define USERCHARS_CLASS "[" USERCHARS "]"
#define PASSCHARS_CLASS "[-[:alnum:]\\Q,?;.:/!%$^*&~\"#'\\E]"
#define HOSTCHARS_CLASS "[-[:alnum:]]"
#define HOST HOSTCHARS_CLASS "+(\\." HOSTCHARS_CLASS "+)*"
#define PORT "(?:\\:[[:digit:]]{1,5})?"
#define PATHCHARS_CLASS "[-[:alnum:]\\Q_$.+!*,:;@&=?/~#%\\E]"
#define PATHTERM_CLASS "[^\\Q]'.:}>) \t\r\n,\"\\E]"
#define SCHEME "(?:news:|telnet:|nntp:|file:\\/|https?:|ftps?:|sftp:|webcal:)"
#define USERPASS USERCHARS_CLASS "+(?:" PASSCHARS_CLASS "+)?"
#define URLPATH   "(?:(/"PATHCHARS_CLASS"+(?:[(]"PATHCHARS_CLASS"*[)])*"PATHCHARS_CLASS"*)*"PATHTERM_CLASS")?"

typedef enum {
  FLAVOR_AS_IS,
  FLAVOR_DEFAULT_TO_HTTP,
  FLAVOR_EMAIL,
  FLAVOR_LP
} UrlRegexFlavor;

typedef struct {
  const char        *pattern;
  UrlRegexFlavor     flavor;
  GRegexCompileFlags flags;
} UrlRegexPattern;

static UrlRegexPattern url_regex_patterns[] = {
  { SCHEME "//(?:" USERPASS "\\@)?" HOST PORT URLPATH, FLAVOR_AS_IS, G_REGEX_CASELESS },
  { "(?:www|ftp)" HOSTCHARS_CLASS "*\\." HOST PORT URLPATH, FLAVOR_DEFAULT_TO_HTTP, G_REGEX_CASELESS},
  { "(?:mailto:)?" USERCHARS_CLASS "[" USERCHARS ".]*\\@" HOSTCHARS_CLASS "+\\." HOST, FLAVOR_EMAIL, G_REGEX_CASELESS  },
  { "(?:lp: #)([[:digit:]]+)", FLAVOR_LP, G_REGEX_CASELESS}
};

static GRegex         **url_regexes;
static UrlRegexFlavor  *url_regex_flavors;
static guint            n_url_regexes;

static char *urlregex_expand(GMatchInfo *match_info, UrlRegexFlavor flavor);

/**
 * urlregex_init:
 *
 * Compiles all of the url matching regular expressions.
 * FIXME: Return immediately or error if initialized more than once
 **/
void
urlregex_init(void)
{
  guint i;

  n_url_regexes = G_N_ELEMENTS(url_regex_patterns);
  url_regexes = g_new0(GRegex*, n_url_regexes);
  url_regex_flavors = g_new0(UrlRegexFlavor, n_url_regexes);

  for (i = 0; i < n_url_regexes; i++) {
    GError *error = NULL;

    url_regexes[i] = g_regex_new(url_regex_patterns[i].pattern,
        url_regex_patterns[i].flags | G_REGEX_OPTIMIZE, 0, &error);

    if (error != NULL) {
      g_message("%s", error->message);
      g_error_free(error);
    }

    url_regex_flavors[i] = url_regex_patterns[i].flavor;
  }
}

/**
 * urlregex_count:
 *
 * Returns the number of available url patterns.
 **/
guint
urlregex_count(void)
{
  return n_url_regexes;
}

/**
 * urlregex_split:
 * @text: the text to split
 * @index: the pattern to use
 *
 * Splits the text into a list of MatchGroup objects.
 **/
GList *
urlregex_split(const char *text, guint index)
{
  GList *result = NULL;
  GRegex *pattern = url_regexes[index];
  GMatchInfo *match_info;
  int text_length = strlen(text);

  int start_pos = 0;
  int end_pos = 0;
  int last_pos = 0;
  int len = 0;

  gchar *token;
  gchar *expanded;

  g_regex_match(pattern, text, 0, &match_info);

  while (g_match_info_matches(match_info)) {
    /* Append previously unmatched text */
    g_match_info_fetch_pos(match_info, 0, &start_pos, &end_pos);
    len = start_pos - last_pos;
    if (len > 0) {
      token = g_strndup(text + last_pos, len);
      result = g_list_append(result, urlregex_matchgroup_new(token, token, NOT_MATCHED));
      g_free(token);
    }

    /* Append matched text */
    token = urlregex_expand(match_info, FLAVOR_AS_IS);
    expanded = urlregex_expand(match_info, url_regex_flavors[index]);
    result = g_list_append(result, urlregex_matchgroup_new(token, expanded, MATCHED));
    g_free(token);
    g_free(expanded);

    g_match_info_next(match_info, NULL);
    last_pos = end_pos;
  }
  /* Append the text after the last match */
  if (last_pos < text_length) {
    token = g_strdup(text + last_pos);
    result = g_list_append(result, urlregex_matchgroup_new(token, token, NOT_MATCHED));
    g_free(token);
  }

  g_match_info_free(match_info);

  return result;
}

/**
 * urlregex_expand:
 * @match_info: describes the matched url
 * @flavor: the type of url
 *
 * Expands the matched url based on the given flavor.
 **/
static char *
urlregex_expand(GMatchInfo *match_info, UrlRegexFlavor flavor)
{
  char *t1;
  char *t2;

  switch(flavor) {
    case FLAVOR_DEFAULT_TO_HTTP:
      t1 = g_match_info_fetch(match_info, 0);
      t2 = g_strconcat(HTTP_BASE_URL, t1, NULL);
      g_free(t1);
      return t2;
    case FLAVOR_EMAIL:
      t1 = g_match_info_fetch(match_info, 0);
      if (!g_str_has_prefix(t1, MAILTO_BASE_URL)) {
        t2 = g_strconcat(MAILTO_BASE_URL, t1, NULL);
        g_free(t1);
        return t2;
      }
      else
        return t1;
    case FLAVOR_LP:
      t1 = g_match_info_fetch(match_info, 1);
      t2 = g_strconcat(LP_BUG_BASE_URL, t1, NULL);
      g_free(t1);
      return t2;
    default:
      return g_match_info_fetch(match_info, 0);
  }
}

/**
 * urlregex_split_all:
 * @text: the text to split
 *
 * Splits the text into a list of MatchGroup objects, applying each url pattern
 * available in order to each of the unmatched sections, keeping the list flat.
 **/
GList *
urlregex_split_all(const char *text)
{
  GList *result = NULL;
  GList *temp = NULL;
  guint i;

  result = g_list_append(result, urlregex_matchgroup_new(text, text, NOT_MATCHED));

  /* Apply each regex in order to sections that haven't yet been matched */
  for (i = 0; i < n_url_regexes; i++) {
    GList *item;
    temp = NULL;
    for (item = result; item; item = item->next) {
      MatchGroup *group = (MatchGroup *)item->data;
      if (group->type == NOT_MATCHED) {
        GList *list = urlregex_split(group->text, i);
        GList *subitem;
        for (subitem = list; subitem; subitem = subitem->next) {
          MatchGroup *subgroup = (MatchGroup *)subitem->data;
          temp = g_list_append(temp, subgroup);
        }
        g_list_free(list);
        urlregex_matchgroup_free(group);
      }
      else {
        temp = g_list_append(temp, group);
      }
    }
    g_list_free(result);
    result = temp;
  }

  return result;
}

/**
 * urlregex_matchgroup_new:
 * @text: the original text
 * @expanded: the expanded url
 * @type: whether this is a matched or unmatched group
 *
 * Creates a new MatchGroup object.
 **/
MatchGroup *
urlregex_matchgroup_new(const char *text, const char *expanded, MatchType type)
{
  MatchGroup *result = g_new0(MatchGroup, 1);
  result->text = g_strdup(text);
  /* TODO: Save space using same data if text == expanded? */
  result->expanded = g_strdup(expanded);
  result->type = type;
  return result;
}

/**
 * urlregex_matchgroup_free:
 * @group: the match group
 *
 * Frees the MatchGroup object.
 **/
void
urlregex_matchgroup_free(MatchGroup *group)
{
  g_free(group->expanded);
  group->expanded = NULL;
  g_free(group->text);
  group->text = NULL;
  g_free(group);
}

/**
 * urlregex_matchgroup_list_free:
 * @list: the match group list
 *
 * Frees a list of MatchGroup objects returned from split or split_all.
 **/
void
urlregex_matchgroup_list_free(GList *list)
{
  GList *item;
  for (item = list; item; item = item->next) {
    urlregex_matchgroup_free((MatchGroup *)item->data);
  }
  g_list_free(list);
}

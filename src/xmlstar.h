#ifndef XMLSTAR_H
#define XMLSTAR_H

#include <config.h>
#include <stdlib.h>

#if HAVE_SETMODE && HAVE_DECL_O_BINARY
# include <io.h>
# include <fcntl.h>
# define set_stdout_binary() setmode(1, O_BINARY)
#else
# define set_stdout_binary()
#endif

#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlreader.h>

typedef enum { /* EXIT_SUCCESS = 0, EXIT_FAILURE = 1, */
    EXIT_BAD_ARGS = EXIT_FAILURE+1, EXIT_BAD_FILE,
    EXIT_LIB_ERROR, EXIT_INTERNAL_ERROR } exit_status;

#define COUNT_OF(array) (sizeof(array)/sizeof(*array))

typedef enum { QUIET, VERBOSE } Verbosity;
typedef enum { CONTINUE, STOP } ErrorStop;

typedef struct _errorInfo {
    const char *filename; /* file error occured in, if any, else NULL */
    xmlTextReaderPtr xmlReader;
    Verbosity verbose;
    ErrorStop stop;
} ErrorInfo;

#if LIBXML_VERSION >= 21200
typedef const xmlError xmlConstError;
#else
typedef xmlError xmlConstError;
#endif

void reportError(void *ptr, xmlConstError *error);
void suppressErrors(void);

typedef struct _gOptions {
    int quiet;            /* no error output */
    int doc_namespace;   /* extract namespace bindings from input doc */
} gOptions;

typedef gOptions *gOptionsPtr;

extern gOptions globalOptions;

void registerXstarVariable(xmlXPathContextPtr ctxt,
    const char* name, xmlXPathObjectPtr value);
void registerXstarNs(xmlXPathContextPtr ctxt);

extern const xmlChar *default_ns;

int parseNSArr(xmlChar** ns_arr, int* plen, int argc, char **argv);
void cleanupNSArr(xmlChar **ns_arr);
extern xmlChar *ns_arr[];

xmlDocPtr readXml(const char *filename, int options);
xmlDocPtr readHtml(const char *filename, int options);

#endif  /* XMLSTAR_H */

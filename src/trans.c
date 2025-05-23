/*  $Id: trans.c,v 1.19 2004/11/22 02:28:21 mgrouch Exp $  */

#include <config.h>
#include "trans.h"
#include "xmlstar.h"

/*
 *  This code is based on xsltproc by Daniel Veillard (daniel@veillard.com)
 *  (see also http://xmlsoft.org/)
 */

int errorno = 0;

/**
 *  Initialize global command line options
 */
void
xsltInitOptions(xsltOptionsPtr ops)
{
    ops->noval = 1;
    ops->nonet = 1;
    ops->omit_decl = 0;
    ops->show_extensions = 0;
    ops->noblanks = 0;
    ops->embed = 0;
#ifdef LIBXML_XINCLUDE_ENABLED
    ops->xinclude = 0;
#endif
#ifdef LIBXML_HTML_ENABLED
    ops->html = 0;
#endif
#ifdef LIBXML_CATALOG_ENABLED
    ops->catalogs = 0;
#endif
}

/**
 *  Initialize LibXML
 */
void
xsltInitLibXml(xsltOptionsPtr ops)
{
    /*
     * Register the EXSLT extensions
     */
    exsltRegisterAll();

    if (ops->show_extensions)
    {
        xsltDebugDumpExtensions(stderr);
        exit(EXIT_SUCCESS);
    }

#ifdef LIBXML_XINCLUDE_ENABLED
    /*
     * enable XInclude
     */
    if (ops->xinclude)
        xsltSetXIncludeDefault(1);
#endif

#ifdef LIBXML_CATALOG_ENABLED
    /*
     * enable SGML catalogs
     */
    if (ops->catalogs)
    {
        char *catalogs = getenv("SGML_CATALOG_FILES");
        if (catalogs == NULL)
            fprintf(stderr, "Variable $SGML_CATALOG_FILES not set\n");
        else
            xmlLoadCatalogs(catalogs);
    }
#endif
}

/* get result of XSL transformation */
xmlDocPtr
xsltTransform(xsltOptionsPtr ops, xmlDocPtr doc, const char** params,
            xsltStylesheetPtr cur, const char *filename)
{
    xsltTransformContextPtr ctxt;
    xmlDocPtr res;

    if (ops->omit_decl)
    {
        cur->omitXmlDeclaration = 1;
    }

#ifdef LIBXML_XINCLUDE_ENABLED
    if (ops->xinclude) xmlXIncludeProcess(doc);
#endif

    ctxt = xsltNewTransformContext(cur, doc);
    if (ctxt == NULL) return NULL;

    res = xsltApplyStylesheetUser(cur, doc, params, NULL, NULL, ctxt);
        
    if (ctxt->state == XSLT_STATE_ERROR)
        errorno = 9;
    if (ctxt->state == XSLT_STATE_STOPPED)
        errorno = 10;
    xsltFreeTransformContext(ctxt);
    xmlFreeDoc(doc);
    if (res == NULL)
    {
        fprintf(stderr, "no result for %s\n", filename);
    }
    return res;
}

/**
 *  Run stylesheet on XML document
 */
void
xsltProcess(xsltOptionsPtr ops, xmlDocPtr doc, const char** params,
            xsltStylesheetPtr cur, const char *filename)
{
    xmlDocPtr res = xsltTransform(ops, doc, params, cur, filename);

    if (res && xsltSaveResultToFile(stdout, res, cur) < 0)
    {
        errorno = EXIT_LIB_ERROR;
    }

    xmlFreeDoc(res);
}

/**
 *  run XSLT on documents
 */
int xsltRun(xsltOptionsPtr ops, char* xsl, const char** params,
            int count, char **docs)
{
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, style;
    int i, options = 0, html_opts = 0;

    options = XSLT_PARSE_OPTIONS;
    if (ops->noval)
        options &= ~(XML_PARSE_DTDLOAD | XML_PARSE_DTDATTR);
    if (ops->noblanks) {
        options |= XML_PARSE_NOBLANKS;
        html_opts |= XML_PARSE_NOBLANKS;
    }

    /*
     * Compile XSLT Sylesheet
     */
    style = readXml(xsl, options);
    if (style == NULL)
    {
        fprintf(stderr,  "cannot parse %s\n", xsl);
        cur = NULL;
        errorno = 4;
    }
    else
    {
        if (ops->embed)
        {             
            cur = xsltLoadStylesheetPI(style);
            if (cur != NULL)
            {
                /* it is an embedded stylesheet */
                xsltProcess(ops, style, params, cur, xsl);
                xsltFreeStylesheet(cur);
                cur = NULL;
            }            
            for (i=0; i<count; i++) 
            {
                style = readXml(docs[i], options);
                if (style == NULL)
                {
                    fprintf(stderr, "cannot parse %s\n", docs[i]);
                    cur = NULL;
                    goto done;
                }
                cur = xsltLoadStylesheetPI(style);
                if (cur != NULL)
                {
                    /* it is an embedded stylesheet */
                    xsltProcess(ops, style, params, cur, docs[i]);
                    xsltFreeStylesheet(cur);
                    cur = NULL;
                }
            } 
            goto done;
        }
        
        cur = xsltParseStylesheetDoc(style);
        if (cur != NULL)
        {
            if (cur->errors != 0)
            {
                errorno = 5;
                goto done;
            }
        }
        else
        {
            xmlFreeDoc(style);
            errorno = 5;
            goto done;
        }
    }

    /*
     * run XSLT
     */
    if ((cur != NULL) && (cur->errors == 0))
    {
        for (i=0; i<count; i++)
        {
            doc = NULL;
#ifdef LIBXML_HTML_ENABLED
            if (ops->html) doc = readHtml(docs[i], html_opts);
            else
#endif
            {
                doc = readXml(docs[i], options);
            }

            if (doc == NULL)
            {
                fprintf(stderr, "unable to parse %s\n", docs[i]);
                errorno = 6;
                continue;
            }
            xsltProcess(ops, doc, params, cur, docs[i]);
        }

        if (count == 0)
        {
            /* stdin */
            doc = NULL;
#ifdef LIBXML_HTML_ENABLED
            if (ops->html) doc = readHtml("-", html_opts);
            else
#endif
                doc = readXml("-", options);
            xsltProcess(ops, doc, params, cur, "-");
        }
    }

done:

    /*
     *  Clean up
     */
    if (cur != NULL) xsltFreeStylesheet(cur);

    return(errorno);
}

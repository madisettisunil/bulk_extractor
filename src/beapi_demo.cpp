/*
 * This program demonstrates the bulk_extractor API.
 */

#include "../config.h"                              // from ../config.h
#include "bulk_extractor_api.h"

#include <iostream>
#include <string>

#include <stdio.h>
#ifdef HAVE_ERR_H
#include <err.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#ifdef HAVE_WINDOWS_H
#include <windows.h>
typedef int (__cdecl *MYPROC)(LPWSTR); 
#endif

int be_cb_demo(int32_t flag,
                        uint32_t arg,
                        const char *feature_recorder_name,
                        const char *pos, // forensic path of the feature
                        const char *feature,size_t feature_len,
                        const char *context,size_t context_len)
{
    printf("be_cb_demo(flag=0x%x,arg=0x%x,name=%s)\n",flag,arg,feature_recorder_name);
    printf("  feature [len=%zu]: ",feature_len);fwrite(feature,1,feature_len,stdout);fputc('\n',stdout);
    printf("  context [len=%zu]: ",context_len);fwrite(context,1,context_len,stdout);fputc('\n',stdout);
    puts("");
    return 0;
}

#ifdef HAVE_DLOPEN
void *getsym(void *lib,const char *name)
{
    void *ptr = dlsym(lib,name);
    if(ptr == 0){
        fprintf(stderr,"dlsym('%s'): %s\n",name,dlerror());
        exit(1);
    }
    return ptr;
}
#endif

int main(int argc,char **argv)
{
    if(argc!=2){
        fprintf(stderr,"usage: %s <filename>\n",argv[0]);
        exit(1);
    }
    const char *fname = argv[1];

    std::string libname = "bulk_extractor.so";
    if(libname.find('/')==std::string::npos){
        libname = "./" + libname;               // fedora requires a complete path name
    }

#ifdef HAVE_DLOPEN_PREFLIGHT
    if(!dlopen_preflight(libname.c_str())){
	fprintf(stderr,"dlopen_preflight - cannot open %s: %s",libname.c_str(),dlerror());
        exit(1);
    }
#endif

    void *lib=dlopen(libname.c_str(), RTLD_LAZY);
    if(lib==0){
        fprintf(stderr,"libname=%s\n",libname.c_str());
        fprintf(stderr,"dlopen: %s\n",dlerror());
        exit(1);
    }

    bulk_extractor_open_t        be_open = (bulk_extractor_open_t)getsym(lib, BULK_EXTRACTOR_OPEN);
    bulk_extractor_config_t      be_config = (bulk_extractor_config_t)getsym(lib, BULK_EXTRACTOR_CONFIG);
    bulk_extractor_analyze_dev_t be_analyze_dev = (bulk_extractor_analyze_dev_t)getsym(lib,BULK_EXTRACTOR_ANALYZE_DEV);
    bulk_extractor_analyze_buf_t be_analyze_buf = (bulk_extractor_analyze_buf_t)getsym(lib,BULK_EXTRACTOR_ANALYZE_BUF);
    bulk_extractor_close_t       be_close = (bulk_extractor_close_t)getsym(lib, BULK_EXTRACTOR_CLOSE);

    /* Get a handle */
    BEFILE *bef = (*be_open)(be_cb_demo);

    /* Now configure the scanners */
    (*be_config)(bef,BEAPI_DISABLE_ALL,     "bulk",0); // turn off all scanners


    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "email",0);        // 

    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "accts",0);        // 

    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "exif",0);        // 

    (*be_config)(bef,BEAPI_SCANNER_ENABLE,  "bulk",0);        // enable the bulk scanner
    (*be_config)(bef,BEAPI_FEATURE_DISABLE, "bulk",0);       // disable bulk feature detector
    (*be_config)(bef,BEAPI_MEMHIST_ENABLE,  "bulk", 10);   // enable the bulk memory histogram

    (*be_config)(bef,BEAPI_PROCESS_COMMANDS,"",0);          // process the enable/disable commands

    const char *demo_buf = "ABCDEFG  demo@api.com Just a demo 617-555-1212 ok!";
    (*be_analyze_buf)(bef,(uint8_t *)demo_buf,strlen(demo_buf));  // analyze the buffer

    /* analyze the file */
    std::cerr << "calling ANALYZE_DEV " << fname << "\n";
    (*be_analyze_dev)(bef,fname);                                 // analyze the file
    std::cerr << "calling CLOSE\n";
    (*be_close)(bef);
    std::cerr << "CLOSED\n";

#ifdef HAVE_DLOPEN
    dlclose(lib);
#endif
    return 0;
}


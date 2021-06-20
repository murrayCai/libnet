#include "net.h"
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

#define FILE_PATH_MAX 1024

#ifdef MODULE_CURR
#undef MODULE_CURR
#define MODULE_CURR MODULE_NET_HTTP_RESOURCE
#endif

#define HTTP_RESOURCE_ARR_GET(r,t) (  HRT_PAGE == (t) ? (r)->page : ( HRT_JS == (t) ? (r)->js : (r)->css ) )

int http_resource_free(http_resource_t **resource){
    int ret = 0;
    R(NULL == resource);
    R(NULL == (*resource));

    int i = 0;
    http_resource_item_t *item = NULL;
    // free css
    for(i = 0; i < (*resource)->css->count; i++ ){
        R( arr_get((void **)&item, (*resource)->css,i) );
        if( NULL != item ){
            R(FREE_TN((void **)&(item->data),str_t,item->dataSize ));
            R(FREE_T( &item, http_resource_item_t ));
        }
    }
    if(NULL != (*resource)->css) R(arr_free(&( (*resource)->css)));
    
    // frees js
    for(i = 0; i < (*resource)->js->count; i++ ){
        R( arr_get((void **)&item, (*resource)->js,i) );
        if( NULL != item ){
            R(FREE_TN( &(item->data),str_t,item->dataSize ));
            R(FREE_T( &item, http_resource_item_t ));
        }
    }
    if(NULL != (*resource)->js) R(arr_free(&( (*resource)->js)));
    
    // frees page
    for(i = 0; i < (*resource)->page->count; i++ ){
        R( arr_get((void **)&item, (*resource)->page,i) );
        if( NULL != item ){
            R(FREE_TN( &(item->data),str_t,item->dataSize ));
            R(FREE_T( &item, http_resource_item_t ));
        }
    }
    if(NULL != (*resource)->page) R(arr_free(&( (*resource)->page)));
    if(NULL != (*resource)) R(FREE_T(resource,http_resource_t));

    return ret;
}

int http_resource_init(http_resource_t **resource,http_resource_config_t *config){
    int ret = 0,errRet = 0;
    R(NULL == resource);
    R(NULL != (*resource));
    R(NULL == config);
    
    R(MALLOC_T(resource,http_resource_t));
    G_E(arr_init(&( (*resource)->page ),config->pageInitCount));
    G_E(arr_init(&( (*resource)->js ),config->jsInitCount));
    G_E(arr_init(&( (*resource)->css ),config->cssInitCount));

    return ret;
error:
    errRet = ret;
    if(NULL != (*resource)->css) R(arr_free(&( (*resource)->css)));
    if(NULL != (*resource)->js) R(arr_free(&( (*resource)->js)));
    if(NULL != (*resource)->page) R(arr_free(&( (*resource)->page)));
    if(NULL != (*resource)) R(FREE_T(resource,http_resource_t));

    return errRet;
}


int http_resource_add(http_resource_t *resource,http_resource_type_e type,
        unsigned int index,const char *data,unsigned int dataSize,unsigned int headerSize){
    int ret = 0,errRet = 0;
    R(NULL == resource);
    R(NULL == data);
    R( HTTP_RESOURCE_TYPE_CHECK(type) );

    arr_t *p = HTTP_RESOURCE_ARR_GET(resource,type);
    R(NULL == p);

    if( p->count < index ){
        R(arr_realloc(&p,index + 16));
        if( HRT_PAGE == type ) resource->page = p;
        else if( HRT_JS == type ) resource->js = p;
        else if( HRT_CSS == type ) resource->css = p;
    }

    http_resource_item_t *item = NULL;
    R(MALLOC_T(&item,http_resource_item_t));

    item->data = data;
    item->dataSize = dataSize;
    item->headerSize = headerSize;
    item->expire = time(NULL);

    G_E(arr_set(p,index,item));

    return ret;
error:
    errRet = ret;
    if(NULL != item) R(FREE_T(&item,http_resource_item_t));
    return errRet;
}

int http_resource_get(http_resource_t *resource,http_resource_type_e type,unsigned int index,http_resource_item_t **dist){
    int ret = 0;
    R(NULL == resource);
    R(NULL == dist);
    R(NULL != (*dist));
    R( HTTP_RESOURCE_TYPE_CHECK(type) );

    arr_t *p = HTTP_RESOURCE_ARR_GET(resource,type);
    R( index >= p->count );
    http_resource_item_t *item = NULL;
    R(arr_get((void **)(&item),p,index));
    R(NULL == item);
    R(NULL == item->data);
    R(0 >= item->dataSize);
    (*dist) = item;
    return ret;
}

#define HTTP_RESOURCE_GET_SUFFIX(type) ( HRT_PAGE == (type) ? HRT_PAGE_SUFFIX : ( HRT_CSS_SUFFIX == (type) ? HRT_CSS_SUFFIX : HRT_JS_SUFFIX ) )

#define HTTP_RESOURCE_FILE_NAME_CHECK(name)\
    ({\
        int r = 0,i = 0;size_t size = strlen(name);\
        for(; i < size; i++){\
                if( '0' >  *((name) + i)  && '9' < *((name) + i) ){\
                    r = 1;\
                    break;\
                }else if ( '.' == *((name) + i) ){\
                    break;\
                }\
        }\
        (r);\
     })

int http_resource_load_from_dir(http_resource_t *resource,http_resource_type_e type, const char *dirPath){
    int ret = 0;
    R(NULL == resource);
    R(NULL == resource->page);
    R(NULL == resource->js);
    R(NULL == resource->css);
    R(NULL == dirPath);
    R(access(dirPath,F_OK|R_OK));
    R( HTTP_RESOURCE_TYPE_CHECK(type) );

    DIR *dir = NULL;
    struct dirent *entry = NULL;
    char filePath[FILE_PATH_MAX] = {0};
    struct stat fsMd,fsHtml;
    unsigned int fIndex = 0;
    char *stopAt = NULL;
    FILE *f = NULL;
    unsigned int fSize = 0;
    size_t fReadSize = 0;

    char *data = NULL;
    unsigned int dataSize = 0;
    
    R(NULL == (dir = opendir(dirPath)));

    while( NULL != ( entry = readdir(dir) ) ){
        f = NULL;
        fSize = 0;
        fReadSize = 0;
        fIndex = 0;
        data = NULL;
        dataSize = 0;

        if( 0 == strcmp(entry->d_name,".") || 0 == strcmp(entry->d_name,"..")) continue;
        snprintf(filePath,FILE_PATH_MAX,"%s/%s",dirPath,entry->d_name);
        G_E(access(filePath,F_OK|R_OK)); // no READ access
        G_E(stat(filePath,&fsMd)); // no access
        if( S_ISDIR(fsMd.st_mode) ) continue; // is dir then continue
        if( HTTP_RESOURCE_FILE_NAME_CHECK(entry->d_name) ) continue;
        fIndex = strtol(entry->d_name,&stopAt,10);
        G_E(NULL == (f = fopen(filePath,"r")));
        G_E(fseek(f,0,SEEK_END));
        G_E(-1 == ( fSize = ftell(f) ) );
        G_E(http_rsp_generate(type,NULL,fSize,&data,&dataSize));
        G_E(fseek(f,0,SEEK_SET));
        fReadSize = fread(data + (dataSize - fSize),1,fSize,f);
        G_E(fReadSize != fSize);
        G_E(http_resource_add(resource,type,fIndex,data,dataSize,dataSize - fSize));
        G_E(fclose(f));
        data = NULL;
        f = NULL;
    }

error:
    if(NULL != data) R(FREE_TN(&data,str_t,dataSize));
    if(NULL != f) fclose(f);
    if(NULL != dir) closedir(dir);
    return ret;
}


int http_resource_print(http_resource_t *resource){
    int ret = 0;
    R(NULL == resource);

    int i = 0;
    http_resource_item_t *item = NULL;
    for(;i < resource->page->count; i++){
        item = NULL;
        R(arr_get((void **)&item,resource->page,i));
        if(NULL != item){
            printf("html[%d]\tsize[%u/%u]\texpire[%lu]\n",i,item->headerSize,item->dataSize,item->expire);
        }
    }
    for(i = 0;i < resource->js->count; i++){
        item = NULL;
        R(arr_get((void **)&item,resource->js,i));
        if(NULL != item){
            printf("js[%d]\tsize[%u/%u]\texpire[%lu]\n",i,item->headerSize,item->dataSize,item->expire);
        }
    }
    for(i = 0;i < resource->css->count; i++){
        item = NULL;
        R(arr_get((void **)&item,resource->css,i));
        if(NULL != item){
            printf("css[%d]\tsize[%u/%u]\texpire[%lu]\n",i,item->headerSize,item->dataSize,item->expire);
        }
    }

    return ret;
}

/*
Copyright (c) 2003-2006 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#undef MAX
#undef MIN

#define MAX(x,y) ((x)<=(y)?(y):(x))
#define MIN(x,y) ((x)<=(y)?(x):(y))

struct _HTTPRequest;

#if defined(USHRT_MAX) && CHUNK_SIZE <= USHRT_MAX
typedef unsigned short chunk_size_t;
#else
typedef unsigned int chunk_size_t;
#endif

typedef struct _Chunk {
    short int locked;
    chunk_size_t size;
    char *data;
} ChunkRec, *ChunkPtr;

struct _Object;

typedef int (*RequestFunction)(struct _Object *, int, int, int,
                               struct _HTTPRequest*, void*);

/* object->type */
enum ObjectType {
    OBJECT_TYPE_INVALID = -1,
    OBJECT_TYPE_HTTP = 1,
    OBJECT_TYPE_DNS = 2
};

typedef struct _Object {
    short refcount;
    enum ObjectType type;
    RequestFunction request;
    void *request_closure;
    void *key;
    unsigned short key_size;
    unsigned short flags;
    unsigned short code;
    void *abort_data;
    struct _Atom *message;
    int length;
    time_t date;
    time_t age;
    time_t expires;
    time_t last_modified;
    time_t atime;
    char *etag;
    unsigned short cache_control;
    int max_age;
    int s_maxage;
    struct _Atom *headers;
    struct _Atom *via;
    int size;
    int numchunks;
    ChunkPtr chunks;
    void *requestor;
    struct _Condition condition;
    struct _DiskCacheEntry *disk_entry;
    struct _Object *next, *previous;
} ObjectRec, *ObjectPtr;

typedef struct _CacheControl {
    int flags;
    int max_age;
    int s_maxage;
    int min_fresh;
    int max_stale;
} CacheControlRec, *CacheControlPtr;

extern int cacheIsShared;
extern int mindlesslyCacheVary;

extern CacheControlRec no_cache_control;
extern int objectExpiryScheduled;
extern int publicObjectCount;
extern int privateObjectCount;
extern int idleTime;

extern const time_t time_t_max;

extern int publicObjectLowMark, objectHighMark;

extern int log2ObjectHashTableSize;

/* object->flags */
enum {
    /* object is public */
    OBJECT_FLAG_PUBLIC = 1,
    /* object hasn't got any headers yet */
    OBJECT_FLAG_INITIAL = 1 << 1,
    /* a server connection is already taking care of the object */
    OBJECT_FLAG_INPROGRESS = 1 << 2,
    /* the object has been superseded -- don't try to fetch it */
    OBJECT_FLAG_SUPERSEDED = 1 << 3,
    /* the object is private and aditionally can only be used by its requestor */
    OBJECT_FLAG_LINEAR = 1 << 4,
    /* the object is currently being validated */
    OBJECT_FLAG_VALIDATING = 1 << 5,
    /* object has been aborted */
    OBJECT_FLAG_ABORTED = 1 << 6,
    /* last object request was a failure */
    OBJECT_FLAG_FAILED = 1 << 7,
    /* Object is a local file */
    OBJECT_FLAG_LOCAL = 1 << 8,
    /* The object's data has been entirely written out to disk */
    OBJECT_FLAG_DISK_ENTRY_COMPLETE = 1 << 9,
    /* The object is suspected to be dynamic -- don't PMM */
    OBJECT_FLAG_DYNAMIC = 1 << 10,
    /* Used for synchronisation between client and server. */
    OBJECT_FLAG_MUTATING = 1 << 11,
};

/* object->cache_control and connection->cache_control */
/* RFC 2616 14.9 */
enum {
    /* Non-standard: like no-cache, but kept internally */
    CACHE_CONTROL_FLAG_NO_HIDDEN = 1,

    /* Directives from RFC 2626 Section 14.9: */
    CACHE_CONTROL_FLAG_NO_CACHE = 1 << 1, /* no-cache */
    CACHE_CONTROL_FLAG_PUBLIC = 1 << 2, /* public */
    CACHE_CONTROL_FLAG_PRIVATE = 1 << 3, /* private */
    CACHE_CONTROL_FLAG_NO_STORE = 1 << 4, /* no-store */
    CACHE_CONTROL_FLAG_NO_TRANSFORM = 1 << 5, /* no-transform */
    CACHE_CONTROL_FLAG_MUST_REVALIDATE = 1 << 6, /* must-revalidate */
    CACHE_CONTROL_FLAG_PROXY_REVALIDATE = 1 << 7, /* proxy-revalidate */
    CACHE_CONTROL_FLAG_ONLY_IF_CACHED = 1 << 8, /* only-if-cached */

    /* set if Vary header; treated as no-cache */
    CACHE_CONTROL_FLAG_VARY = 1 << 9,
    /* set if Authorization header; treated specially */
    CACHE_CONTROL_FLAG_AUTHORIZATION = 1 << 10,
    /* set if cookie */
    CACHE_CONTROL_FLAG_COOKIE = 1 << 11,
    /* set if this object should never be combined with another resource */
    CACHE_CONTROL_FLAG_MISMATCH = 1 << 12
};
struct _HTTPRequest;

void preinitObject(void);
void initObject(void);
ObjectPtr findObject(int type, const void *key, int key_size);
ObjectPtr makeObject(enum ObjectType type, const void *key, int key_size,
                     int public, int fromdisk,
                     int (*request)(ObjectPtr, int, int, int, 
                                    struct _HTTPRequest*, void*), void*);
void objectMetadataChanged(ObjectPtr object, int dirty);
ObjectPtr retainObject(ObjectPtr);
void releaseObject(ObjectPtr);
int objectSetChunks(ObjectPtr object, int numchunks);
void lockChunk(ObjectPtr, int);
void unlockChunk(ObjectPtr, int);
void destroyObject(ObjectPtr object);
void privatiseObject(ObjectPtr object, int linear);
void abortObject(ObjectPtr object, int code, struct _Atom *message);
void supersedeObject(ObjectPtr);
void notifyObject(ObjectPtr);
void releaseNotifyObject(ObjectPtr);
ObjectPtr objectPartial(ObjectPtr object, int length, struct _Atom *headers);
int objectHoleSize(ObjectPtr object, int offset)
    ATTRIBUTE ((pure));
int objectHasData(ObjectPtr object, int from, int to)
    ATTRIBUTE ((pure));
int objectAddData(ObjectPtr object, const char *data, int offset, int len);
void objectPrintf(ObjectPtr object, int offset, const char *format, ...)
     ATTRIBUTE ((format (printf, 3, 4)));
int discardObjectsHandler(TimeEventHandlerPtr);
void writeoutObjects(int);
int discardObjects(int all, int force);
int objectIsStale(ObjectPtr object, CacheControlPtr cache_control)
    ATTRIBUTE ((pure));
int objectMustRevalidate(ObjectPtr object, CacheControlPtr cache_control)
    ATTRIBUTE ((pure));

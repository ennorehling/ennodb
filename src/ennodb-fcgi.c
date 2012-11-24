#include <fcgiapp.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <critbit.h>

#define KEYSIZE 256
#define VALSIZE 32768

/* const char * content_type = "application/octet-stream"; */
const char * content_type = "text/plain";

static void do_create(critbit_tree *cb, const char *key, FCGX_Stream *in, FCGX_Stream *out)
{
  size_t keylen = strlen(key);
  int n;
  char dst[VALSIZE+KEYSIZE+1+sizeof(int)];

  memcpy(dst, key, keylen);
  dst[keylen] = 0;
  n = FCGX_GetStr(dst+keylen+1+sizeof(int), VALSIZE, in);
  *(int *)(dst+keylen+1) = n;
  if (cb_insert(cb, dst, n+keylen+1+sizeof(int))) {
    FCGX_FPrintF(out, "Status: 201 Created\r\n\r\n");    
  }
}

static void do_read(critbit_tree *cb, const char *key, FCGX_Stream *out)
{
  int result;
  size_t keylen = strlen(key);
  const void * match;
  result = cb_find_prefix(cb, key, keylen+1, &match, 1, 0);
  if (result>0) {
    const char * data = (const char *)match;
    int n = *(int*)(data+keylen+1);
    FCGX_FPrintF(out, "Content-type: %s\r\n\r\n", content_type);
    FCGX_PutStr(data+keylen+1+sizeof(int), n, out);
  } else {
    FCGX_FPrintF(out, "Status: 404 Not Found\r\n\r\n");
  }
}

int main(int argc, char ** argv)
{
  FCGX_Stream *in, *out, *err;
  FCGX_ParamArray envp;
  critbit_tree cb = CRITBIT_TREE();

  while (FCGX_Accept(&in, &out, &err, &envp) >= 0) {
    const char *request_method = 0;
    const char *script_name = 0;
    const char *key;

    script_name = FCGX_GetParam("SCRIPT_FILENAME", envp);
    key = strrchr(script_name, '/');
    if (!key || strlen(key)<=5) {
      FCGX_FPrintF(out, "Status: 404 Not Found\r\n\r\n");
      continue;
    }
    ++key;
    request_method = FCGX_GetParam("REQUEST_METHOD", envp);
    if (strcmp(request_method, "GET")==0) {
      /* read */
      do_read(&cb, key, out);
    }
    else if (strcmp(request_method, "PUT")==0) {
      /* create */
      do_create(&cb, key, in, out);
    } else {
      FCGX_FPrintF(out, "Status: 405 Method Not Supported\r\n\r\n");
    }
  } /* while */
  
  return 0;
}

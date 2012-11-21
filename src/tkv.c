#include <fcgiapp.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <critbit.h>

int main(int argc, char ** argv)
{
  FCGX_Stream *in, *out, *err;
  FCGX_ParamArray envp;
  int count = 0;
/*  critbit_tree cb = CRITBIT_TREE(); */
  
  while (FCGX_Accept(&in, &out, &err, &envp) >= 0) {
    char *contentLength = FCGX_GetParam("CONTENT_LENGTH", envp);
    int len = 0;
    const char *request_method = 0, *query_string = 0;
/*    const char *document_root = 0, *script_filename = 0; */

    FCGX_FPrintF(out,
                 "Content-type: text/plain\r\n"
                 "\r\n"
                 "FastCGI echo (fcgiapp version)"
                 "Request number %d,  Process ID: %d\n", ++count, getpid());
    
    query_string = FCGX_GetParam("QUERY_STRING", envp);
    request_method = FCGX_GetParam("REQUEST_METHOD", envp);
    FCGX_FPrintF(out, "QUERY:   %s\n", query_string);
    FCGX_FPrintF(out, "REQUEST: %s\n", request_method);

    if (contentLength != NULL)
      len = strtol(contentLength, NULL, 10);
    
    if (len <= 0) {
      FCGX_FPrintF(out, "No data from standard input.\n");
    }
    else {
      int i, ch;
      
      FCGX_FPrintF(out, "Standard input:\n\n");
      for (i = 0; i < len; i++) {
        if ((ch = FCGX_GetChar(in)) < 0) {
          FCGX_FPrintF(out,
                       "Error: Not enough bytes received on standard input\n");
          break;
        }
        FCGX_PutChar(ch, out);
      }
      FCGX_FPrintF(out, "\n\n");
    }
  } /* while */
  
  return 0;
}

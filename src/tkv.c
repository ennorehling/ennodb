#include <fcgiapp.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <critbit.h>

extern char **environ;

static void PrintEnv(FCGX_Stream *out, char *label, char **envp)
{
  FCGX_FPrintF(out, "%s:<br>\n<pre>\n", label);
  for( ; *envp ; envp++) {
    FCGX_FPrintF(out, "%s\n", *envp);
  }
  FCGX_FPrintF(out, "</pre><p>\n");
}

int main(int argc, char ** argv)
{
  FCGX_Stream *in, *out, *err;
  FCGX_ParamArray envp;
  int count = 0;
  
  while (FCGX_Accept(&in, &out, &err, &envp) >= 0) {
    char *contentLength = FCGX_GetParam("CONTENT_LENGTH", envp);
    char **env;
    int len = 0;
    const char *server_name = 0, *request_method = 0;
    
    for (env = envp ; *env ; ++env) {
      if (strncmp(*env, "REQUEST_METHOD=", 15)==0) {
        request_method = (*env)+15;
      }
      else if (strncmp(*env, "SERVER_NAME=", 12)==0) {
        server_name = (*env)+12;
      }
    }

    FCGX_FPrintF(out,
                 "Content-type: text/html\r\n"
                 "\r\n"
                 "<title>FastCGI echo (fcgiapp version)</title>"
                 "<h1>FastCGI echo (fcgiapp version)</h1>\n"
                 "Request number %d,  Process ID: %d<p>\n", ++count, getpid());
    
    FCGX_FPrintF(out, "SERVER:  %s<p>\n", server_name);
    FCGX_FPrintF(out, "REQUEST: %s<p>\n", request_method);

    if (contentLength != NULL)
            len = strtol(contentLength, NULL, 10);
    
    if (len <= 0) {
      FCGX_FPrintF(out, "No data from standard input.<p>\n");
    }
    else {
      int i, ch;
      
      FCGX_FPrintF(out, "Standard input:<br>\n<pre>\n");
      for (i = 0; i < len; i++) {
        if ((ch = FCGX_GetChar(in)) < 0) {
          FCGX_FPrintF(out,
                       "Error: Not enough bytes received on standard input<p>\n");
          break;
        }
        FCGX_PutChar(ch, out);
      }
      FCGX_FPrintF(out, "\n</pre><p>\n");
    }
    
    PrintEnv(out, "Request environment", envp);
    PrintEnv(out, "Initial environment", environ);
  } /* while */
  
  return 0;
}
